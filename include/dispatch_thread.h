// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef INCLUDE_DISPATCH_THREAD_H_
#define INCLUDE_DISPATCH_THREAD_H_

#include <set>
#include <queue>

#include "include/xdebug.h"
#include "include/worker_thread.h"
#include "include/server_thread.h"
#include "include/pink_epoll.h"

namespace pink {
template <typename T>
class DispatchThread : public ServerThread {
 public:
  // This type Dispatch thread just get Connection and then Dispatch the fd to
  // worker thead
  /**
   * @brief
   *
   * @param port the port number
   * @param work_num
   * @param worker_thread the worker thred we define
   * @param cron_interval the cron job interval
   */
  DispatchThread(int port, int work_num, WorkerThread<T> **worker_thread, int cron_interval = 0) :
    ServerThread::ServerThread(port, cron_interval),
    last_thread_(0),
    work_num_(work_num),
    worker_thread_(worker_thread) {
    StartWorkerThreads();
  }

  /**
   * @brief
   *
   * @param ip the ip string
   * @param port the port number
   * @param work_num
   * @param worker_thread the worker thred we define
   * @param cron_interval the cron job interval
   */

  DispatchThread(const std::string &ip, int port, int work_num, WorkerThread<T> **worker_thread, int cron_interval = 0) :
    ServerThread::ServerThread(ip, cron_interval),
    last_thread_(0),
    work_num_(work_num),
    worker_thread_(worker_thread) {
    StartWorkerThreads();
  }

  DispatchThread(const std::set<std::string>& ips, int port, int work_num, WorkerThread<T> **worker_thread, int cron_interval = 0) :
    ServerThread::ServerThread(ips, cron_interval),
    last_thread_(0),
    work_num_(work_num),
    worker_thread_(worker_thread) {
    StartWorkerThreads();
  }

  int work_num() {
    return work_num_;
  }

  WorkerThread<T>** worker_thread() {
    return worker_thread_;
  }

  void StartWorkerThreads() {
    for (int i = 0; i < work_num_; i++) {
      worker_thread_[i]->StartThread();
    }
  }

  virtual ~DispatchThread() {
  }

  virtual void CronHandle() {
  }

  virtual bool AccessHandle(std::string& ip) {
    return true;
  }

  virtual void HandleNewConn(const int& connfd, const std::string& ip_port) {
    std::queue<PinkItem> *q = &(worker_thread_[last_thread_]->conn_queue_);
    PinkItem ti(connfd, ip_port);
    {
      MutexLock l(&worker_thread_[last_thread_]->mutex_);
      q->push(ti);
    }
    write(worker_thread_[last_thread_]->notify_send_fd(), "", 1);
    last_thread_++;
    last_thread_ %= work_num_;
  }

  virtual void HandleConnEvent(PinkFiredEvent *pfe) {
  }

 private:
  /*
   * Here we used auto poll to find the next work thread,
   * last_thread_ is the last work thread
   */
  int last_thread_;
  int work_num_;
  /*
   * This is the work threads
   */
  WorkerThread<T> **worker_thread_;

  // No copying allowed
  DispatchThread(const DispatchThread&);
  void operator=(const DispatchThread&);
};

}// namespace pink

#endif // INCLUDE_DISPATCH_THREAD_H_
