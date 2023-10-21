#include "dagger.h"

#include <gtest/gtest.h>
#include <memory>

TEST(Dagger, Basic) {
  dagger::DagManager mgr(10);
  auto node1 = std::make_unique<dagger::Node>(
      "1", []() { printf("1\n"); }, std::vector<dagger::Node *>{});
  auto node2 = std::make_unique<dagger::Node>(
      "2", []() { printf("2\n"); }, std::vector<dagger::Node *>{node1.get()});
  auto node3 = std::make_unique<dagger::Node>(
      "3", []() { printf("3\n"); }, std::vector<dagger::Node *>{node2.get()});
  auto node4 = std::make_unique<dagger::Node>(
      "4", []() { printf("4\n"); }, std::vector<dagger::Node *>{node3.get()});
  auto node5 = std::make_unique<dagger::Node>(
      "5", []() { printf("5\n"); }, std::vector<dagger::Node *>{node4.get()});
  mgr.AddNode(std::move(node1));
  mgr.AddNode(std::move(node2));
  mgr.AddNode(std::move(node3));
  mgr.AddNode(std::move(node4));
  mgr.AddNode(std::move(node5));
  mgr.BlockingRun();
}
