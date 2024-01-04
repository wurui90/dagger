#include "dagger.h"

namespace dagger {
bool Node::IsReadyToEnqueue() const {
  if (GetState() != NodeState::kNotInQueue) {
    return false;
  }
  for (Node *in_node : in_nodes_) {
    if (in_node->GetState() != NodeState::kComplete) {
      return false;
    }
  }
  return true;
}

void DagManager::BlockingRun() {
  // Start the thread pool.
  for (int i = 0; i < thread_pool_size_; i++) {
    thread_pool_.emplace_back(&DagManager::ThreadWorker, this);
  }

  while (!AreAllNodesComplete()) {
    for (const auto &it : node_id_to_node_) {
      if (it.second->IsReadyToEnqueue()) {
        std::scoped_lock lock(nodes_to_be_started_mu_);
        // Push the out going node into the nodes queue, and modify the out
        // going node's state immediately within the scope of lock to avoid
        // race conditions.
        nodes_to_be_started_.push(it.second.get());
        it.second->SetState(NodeState::kInqueue);
      }
    }
    std::this_thread::yield();
    usleep(1);
  }
}

bool DagManager::AreAllNodesComplete() const {
  for (const auto &it : node_id_to_node_) {
    if (it.second->GetState() != NodeState::kComplete) {
      return false;
    }
  }
  return true;
}

void DagManager::StopAndJoinAllThread() {
  {
    std::scoped_lock lock(stop_thread_mu_);
    stop_thread_ = true;
  }
  for (int i = 0; i < thread_pool_size_; i++) {
    thread_pool_[i].join();
  }
}

void DagManager::ThreadWorker() {
  Node *node = nullptr;
  while (!ShouldStopThread()) {
    {
      std::scoped_lock lock(nodes_to_be_started_mu_);
      if (nodes_to_be_started_.empty()) {
        node = nullptr;
      } else {
        node = nodes_to_be_started_.front();
        nodes_to_be_started_.pop();
      }
    }

    if (node == nullptr) {
      std::this_thread::yield();
      // Can add a usleep(1) here.
      continue;
    }
    node->SetState(NodeState::kRunning);
    node->BlockingRun();
    node->SetState(NodeState::kComplete);
  }
}

} // namespace dagger