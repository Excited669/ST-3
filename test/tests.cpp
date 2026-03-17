// Copyright 2021 GHA Test Team

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "TimedDoor.h"

using ::testing::Return;

class MockTimerClient : public TimerClient {
 public:
  MOCK_METHOD(void, Timeout, (), (override));
};

class MockDoor : public Door {
 public:
  MOCK_METHOD(void, lock, (), (override));
  MOCK_METHOD(void, unlock, (), (override));
  MOCK_METHOD(bool, isDoorOpened, (), (override));
};

class TimerClientInvoker {
 public:
  void Fire(TimerClient* client) {
    client->Timeout();
  }
};

class DoorController {
 public:
  void Close(Door* door) {
    door->lock();
  }

  void Open(Door* door) {
    door->unlock();
  }

  bool State(Door* door) {
    return door->isDoorOpened();
  }
};

class TimedDoorTest : public ::testing::Test {
 protected:
  TimedDoor* door = nullptr;
  DoorTimerAdapter* adapter = nullptr;

  void SetUp() override {
    door = new TimedDoor(0);
    adapter = new DoorTimerAdapter(*door);
  }

  void TearDown() override {
    delete adapter;
    delete door;
  }
};

TEST_F(TimedDoorTest, DoorIsClosedAfterCreation) {
  EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, TimeoutValueIsStored) {
  TimedDoor localDoor(7);
  EXPECT_EQ(localDoor.getTimeOut(), 7);
}

TEST_F(TimedDoorTest, LockMakesDoorClosed) {
  door->lock();
  EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, ThrowStateDoesNotThrowForClosedDoor) {
  door->lock();
  EXPECT_NO_THROW(door->throwState());
}

TEST_F(TimedDoorTest, UnlockWithZeroTimeoutThrowsException) {
  EXPECT_THROW(door->unlock(), std::runtime_error);
}

TEST_F(TimedDoorTest, ThrowStateThrowsForOpenedDoor) {
  EXPECT_THROW(door->unlock(), std::runtime_error);
  EXPECT_TRUE(door->isDoorOpened());
  EXPECT_THROW(door->throwState(), std::runtime_error);
}

TEST_F(TimedDoorTest, AdapterTimeoutDoesNotThrowForClosedDoor) {
  door->lock();
  EXPECT_NO_THROW(adapter->Timeout());
}

TEST_F(TimedDoorTest, AdapterTimeoutThrowsForOpenedDoor) {
  EXPECT_THROW(door->unlock(), std::runtime_error);
  EXPECT_THROW(adapter->Timeout(), std::runtime_error);
}

TEST(TimerTest, RegisterCallsTimeoutForClient) {
  Timer timer;
  MockTimerClient client;

  EXPECT_CALL(client, Timeout()).Times(1);
  timer.tregister(0, &client);
}

TEST(MockInterfacesTest, TimerClientMethodIsCalledThroughHelper) {
  MockTimerClient client;
  TimerClientInvoker invoker;

  EXPECT_CALL(client, Timeout()).Times(1);
  invoker.Fire(&client);
}

TEST(MockInterfacesTest, DoorLockMethodIsCalledThroughHelper) {
  MockDoor door;
  DoorController controller;

  EXPECT_CALL(door, lock()).Times(1);
  controller.Close(&door);
}

TEST(MockInterfacesTest, DoorUnlockMethodIsCalledThroughHelper) {
  MockDoor door;
  DoorController controller;

  EXPECT_CALL(door, unlock()).Times(1);
  controller.Open(&door);
}

TEST(MockInterfacesTest, DoorStateMethodIsCalledThroughHelper) {
  MockDoor door;
  DoorController controller;

  EXPECT_CALL(door, isDoorOpened()).Times(1).WillOnce(Return(true));
  EXPECT_TRUE(controller.State(&door));
}

TEST(IntegrationTest, ClosingDoorBeforeTimeoutPreventsException) {
  TimedDoor door(1);

  std::thread closer([&door]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    door.lock();
  });

  EXPECT_NO_THROW(door.unlock());

  closer.join();
  EXPECT_FALSE(door.isDoorOpened());
}