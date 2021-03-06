/**************************************************************************
 *
 * Copyright (c) 2017-2021, luotang.me <wypx520@gmail.com>, China.
 * All rights reserved.
 *
 * Distributed under the terms of the GNU General Public License v2.
 *
 * This software is provided 'as is' with no explicit or implied warranties
 * in respect of its properties, including, but not limited to, correctness
 * and/or fitness for purpose.
 *
 **************************************************************************/
#include "event_stack.h"

#include <butil/logging.h>

#include <random>

#include "signal_manager.h"

namespace MSF {

EventStack::EventStack() : baseLoop_(EventLoop()), started_(false) {
  InitSigHandler();
}

EventStack::~EventStack() {
  // Don't delete loop, it's stack variable
}

EventLoop *EventStack::getOneLoop() {
  // baseLoop_.assertInLoopThread();
  if (unlikely(!started_)) {
    LOG(FATAL) << "Event loop thread pool not start.";
    return nullptr;
  }

  if (threadArgs_.size() == 0) {
    LOG(INFO) << "Event loop return baseloop.";
    return &baseLoop_;
  }

  std::default_random_engine e;
  std::uniform_int_distribution<unsigned> u(
      0, threadArgs_.size() - 1);  //随机数分布对象
  return threadArgs_[u(e)].loop_;
}

EventLoop *EventStack::getHashLoop() {
  // baseLoop_.assertInLoopThread();
  if (unlikely(!started_)) {
    LOG(FATAL) << "Event loop thread pool not start.";
    return nullptr;
  }

  if (threadArgs_.size() == 0) {
    LOG(INFO) << "Event loop return baseloop.";
    return &baseLoop_;
  }
  return threadArgs_[(next_++) % threadArgs_.size()].loop_;
}

EventLoop *EventStack::getBaseLoop() { return &baseLoop_; }

EventLoop *EventStack::GetFixedLoop(uint32_t idx) {
  if (idx < threadArgs_.size()) {
    return threadArgs_[idx].loop_;
  }
  return nullptr;
}
std::vector<EventLoop *> EventStack::getAllLoops() {
  std::vector<EventLoop *> loops;
  for (const auto &th : threadArgs_) {
    loops.push_back(th.loop_);
  }
  return loops;
}

void EventStack::startLoop(ThreadArg *arg) {
  // not 100% race-free, eg. threadFunc could be running callback_.
  if (arg->loop_ != nullptr) {
    if (initCb_) {
      initCb_(arg->loop_);
    }
    arg->loop_->loop();

    // still a tiny chance to call destructed object, if threadFunc exits just
    // now. but when EventLoopThread destructs, usually programming is exiting
    // anyway.
    arg->loop_->quit();
    arg->thread_->join();
    arg->loop_ = nullptr;
  } else {
    LOG(FATAL) << "Event loop pointer is invalid";
  }
}

void EventStack::setThreadArgs(const std::vector<ThreadArg> &threadArg) {
  std::move(threadArg.begin(), threadArg.end(),
            std::back_inserter(threadArgs_));
}

bool EventStack::startThreads(const std::vector<ThreadArg> &threadArgs) {
  if (unlikely(started_)) {
    LOG(FATAL) << "Event loop thread pool is started.";
    return false;
  }
  // baseLoop_->assertInLoopThread();
  started_ = true;

  std::move(threadArgs.begin(), threadArgs.end(),
            std::back_inserter(threadArgs_));

  for (auto &th : threadArgs_) {
    th.thread_ =
        new Thread(std::bind(&EventStack::startLoop, this, &th), th.name_);
    if (th.thread_ == nullptr) {
      LOG(FATAL) << "Alloc event thread failed, errno:" << errno;
      return false;
    }
    th.thread_->start([this, &th]() {
      /* Pre init function before thread loop */
      th.loop_ = new EventLoop();
      if (th.loop_ == nullptr) {
        LOG(FATAL) << "Alloc event loop failed, errno:" << errno;
        return;
      }
    });
  }
  LOG(INFO) << "Start all " << threadArgs_.size() << " thread success.";
  return true;
}

bool EventStack::start(const ThreadInitCallback &cb) {
  if (threadArgs_.size() == 0) {
    if (cb) {
      cb(&baseLoop_);
    }
  }

  baseLoop_.loop();

  return true;
}

}  // namespace MSF
