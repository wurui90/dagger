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
    std::scoped_lock lock(state_mu_);
    return state_;
  }

  void SetState(NodeState state) {
    std::scoped_lock lock(state_mu_);
    state_ = state;
  }

  void BlockingRun() const { func_(); }

  NodeId id() const { return id_; }

  bool IsReadyToEnqueue() const;

private:
  const NodeId id_;
  const FuncType func_;
  const std::vector<Node *> in_nodes_;

  NodeState state_ = NodeState::kNotInQueue;
  mutable std::mutex state_mu_;
};

class DagManager {
public:
  DagManager(int thread_pool_size) : thread_pool_size_(thread_pool_size) {}

  ~DagManager() { StopAndJoinAllThread(); }

  void AddNode(std::unique_ptr<Node> node) {
    NodeId id = node->id();
    node_id_to_node_[id] = std::move(node);
  }

  void BlockingRun();

  bool AreAllNodesComplete() const;

  bool ShouldStopThread() const {
    std::scoped_lock lock(stop_thread_mu_);
    return stop_thread_;
  }

  void StopAndJoinAllThread();

private:
  void ThreadWorker();

  std::unordered_map<NodeId, std::unique_ptr<Node>> node_id_to_node_;

  std::vector<std::thread> thread_pool_;
  const int thread_pool_size_;

  std::queue<Node *> nodes_to_be_started_;
  mutable std::mutex nodes_to_be_started_mu_;

  mutable std::mutex stop_thread_mu_;
  bool stop_thread_ = false;
};

} // namespace dagger