#include <iostream>
#include <string>

int main() {
  std::cout << (bool)std::isalpha(' ') << std::endl;
  std::cout << (bool)std::isalpha('a') << std::endl;
}
