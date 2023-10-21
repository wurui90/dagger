#include <cassert>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace dagger {

using NodeId = std::string;

enum class NodeState {
  kUnknown = 0,
  kNotInQueue = 1,
  kInqueue = 2,
  kRunning = 3,
  kComplete = 4
};

class Node {
public:
  using FuncType = std::function<void()>;

  Node(const NodeId &id, FuncType func, std::vector<Node *> in_nodes)
      : id_(id), func_(func), in_nodes_(in_nodes) {}

  NodeState GetState() const {
    std::scoped_lock lock(mu_);
    return state_;
  }

  void SetState(NodeState state) {
    std::scoped_lock lock(mu_);
    state_ = state;
  }

  void BlockingRun() const { func_(); }

  NodeId id() const { return id_; }

  bool IsReadyToEnqueue() const {
    printf("IsReadyToEnqueue called on %s state %d\n", id().c_str(),
           GetState());
    if (GetState() != NodeState::kNotInQueue) {
      return false;
    }
    for (Node *in_node : in_nodes_) {
      if (in_node->GetState() != NodeState::kComplete) {
        printf("%s is not ready to enqueue\n", id().c_str());
        return false;
      }
    }
    printf("%s is ready to enqueue\n", id().c_str());
    return true;
  }

private:
  NodeId id_;
  FuncType func_;
  NodeState state_ = NodeState::kNotInQueue;
  std::vector<Node *> in_nodes_;

  mutable std::mutex mu_;
};

class DagManager {
public:
  DagManager(int thread_pool_size) : thread_pool_size_(thread_pool_size) {}

  ~DagManager() { StopAndJoinAllThread(); }

  void AddNode(std::unique_ptr<Node> node) {
    NodeId id = node->id();
    printf("node id %s is added\n", id.c_str());
    node_id_to_node_[id] = std::move(node);
  }

  void BlockingRun() {
    // Start the thread pool.
    for (int i = 0; i < thread_pool_size_; i++) {
      thread_pool_.emplace_back(&DagManager::ThreadWorker, this);
    }

    while (!AreAllNodesComplete()) {
      for (const auto &it : node_id_to_node_) {
        printf("node_id_to_node_ it %s\n", it.second->id().c_str());
        if (it.second->IsReadyToEnqueue()) {
          {
            std::scoped_lock lock(nodes_queue_mu_);
            nodes_to_be_started_.push(it.second.get());

            printf("enqueue %s\n", it.second->id().c_str());
            it.second->SetState(NodeState::kInqueue);
          }
        }
      }
      // usleep(1);
      std::this_thread::yield();
    }
  }

  bool AreAllNodesComplete() const {
    for (const auto &it : node_id_to_node_) {
      if (it.second->GetState() != NodeState::kComplete) {
        return false;
      }
    }
    return true;
  }

  bool ShouldStopThread() const {
    std::scoped_lock lock(stop_thread_mu_);
    return stop_thread_;
  }

  void StopAndJoinAllThread() {
    {
      std::scoped_lock lock(stop_thread_mu_);
      stop_thread_ = true;
    }
    for (int i = 0; i < thread_pool_size_; i++) {
      thread_pool_[i].join();
    }
  }

private:
  void ThreadWorker() {
    Node *node = nullptr;
    while (!ShouldStopThread()) {
      {
        std::scoped_lock lock(nodes_queue_mu_);
        if (nodes_to_be_started_.empty()) {
          node = nullptr;
        } else {
          node = nodes_to_be_started_.front();
          nodes_to_be_started_.pop();
        }
      }

      if (node == nullptr) {
        std::this_thread::yield();
        // usleep(1);
        continue;
      }
      assert(node != nullptr);
      node->SetState(NodeState::kRunning);
      node->BlockingRun();
      node->SetState(NodeState::kComplete);
      printf("%s is set to complete\n", node->id().c_str());
    }
  }

  std::unordered_map<NodeId, std::unique_ptr<Node>> node_id_to_node_;
  std::queue<Node *> nodes_to_be_started_;
  std::vector<std::thread> thread_pool_;
  const int thread_pool_size_;
  bool stop_thread_ = false;
  mutable std::mutex nodes_queue_mu_;
  mutable std::mutex stop_thread_mu_;
};

} // namespace dagger