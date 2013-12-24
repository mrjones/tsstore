#include <iostream>

#include "tsstore.h"

int main(int argc, char* argv[]) {
  TSStore store;
  std::cout << "Hello " << store.foo() << std::endl;
}
