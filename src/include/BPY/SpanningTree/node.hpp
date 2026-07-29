

 #pragma once
 #include <vector>
 #include <string>
 #include <iostream>

 struct Node {
     int id;
     int localValue;
     Node* parent;
     std::vector<Node*> children;

      //convergecast state
     int pendingChildren = 0;
     int accumulated = 0;

     explicit Node(int id, Node* parent)
     : id(id), localValue(id * 10), parent(parent) {}

     bool isLeaf() const noexcept { return children.empty(); }
     bool isRoot() const noexcept { return parent == nullptr; }

     void resetState() {
         accumulated = 0;
         pendingChildren = static_cast<int>(children.size());
     }

     void start(const std::string& msg) {
         std::cout << "Node " << id << " received: " << msg << "\n";

         resetState();

         if (isLeaf()) {
             parent ? parent->back(localValue) : printResult(localValue);
             return;
         }

         for (Node* c : children)
             c->start(msg);
     }

     void back(int value) {
         accumulated += value;
         pendingChildren--;

         if (pendingChildren == 0) {
             int total = accumulated + localValue;

             if (isRoot()) {
                 printResult(total);
             } else {
                 parent->back(total);
             }
         }
     }

 private:
     void printResult(int total) {
         std::cout << "\n>>> CONVERGECAST COMPLETE <<<\n";
         std::cout << "Root " << id << " total sum = " << total << "\n\n";
     }
 };

/*
  #include <vector>
  #include <string>
  #include <iostream>

  struct Node {
      int id;
      int localValue;
      Node* parent{nullptr};
      std::vector<Node*> child;

      size_t responsesReceived = 0;
      int accumulatedValue = 0;

      Node(int _id, Node* _parent)
          : id(_id), localValue(_id * 10), parent(_parent) {}

      bool isRoot() const { return parent == nullptr; }

      void go(const std::string& msg) {
          std::cout << "Node " << id << " received: " << msg << std::endl;

           Special Case: I am a leaf (no children)
          if (child.empty()) {
              this->back(localValue);
              return;
          }

           Recursive Step: Tell all children to 'go'
          for (Node* c : child) {
              c->go(msg);
          }
      }

      void back(int value) {
           If this is a self-call from a leaf, 'value' is localValue.
           If this is from a child, 'value' is that child's total subtree sum.

          if (isRoot()) {
               Root logic
              if (child.empty()) {
                   Root is the only node in the tree
                  printFinalResult(localValue);
              } else {
                  accumulatedValue += value;
                  responsesReceived++;
                  if (responsesReceived == child.size()) {
                      printFinalResult(accumulatedValue + localValue);
                       Reset
                      accumulatedValue = 0;
                      responsesReceived = 0;
                  }
              }
          } else {
               Internal Node logic
              accumulatedValue += value;
              responsesReceived++;

              if (responsesReceived == child.size()) {
                  int totalToSend = accumulatedValue + localValue;
                   Reset before moving up
                  accumulatedValue = 0;
                  responsesReceived = 0;
                  parent->back(totalToSend);
              }
          }
      }

  private:
      void printFinalResult(int total) {
          std::cout << "\n>>> CONVERGECAST COMPLETE <<<\n";
          std::cout << "Root (ID " << id << ") Total Network Sum: " << total << std::endl;
      }
  };*/
