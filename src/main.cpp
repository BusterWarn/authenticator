#include <iostream>

#include "authenticator_api.h"

int main()
{
  std::cout << "What would you like to do? Print users (P) Add user (A) Remove user (R)" << std::endl;
  char user_choice;
  std::cin >> user_choice;

  switch (user_choice)
  {
    case 'p':
    case 'P':
    {
      auto users = authenticator_api::get_users();
      for (auto user : users)
      {
        std::cout << user << '\n';
      }
      break;
    }
    case 'a':
    case 'A':
    {
      std::string username;
      std::string password;
      std::cout << "username\n";
      std::cin >> username;
      std::cout << "password\n";
      std::cin >> password;

      authenticator_api::add_user(username, "id", "User", password);
      break;
    }
    case 'r':
    case 'R':
    {
      std::string username;
      std::cout << "username\n";
      std::cin >> username;

      authenticator_api::remove_user(username);
      break;
    }
    default:
      std::cout << "Invalid input: " << user_choice << '\n';
      break;
  }

  return 0;
}