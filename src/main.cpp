#include <iostream>

#include "authenticator_api.h"

int main()
{
  bool run_loop = true;
  std::uint64_t authorization = 0;
  while (run_loop)
  {
    std::cout << "\nWhat would you like to do? Print users (P) Add user (A) Remove user (R) Login (L) Quit (Q)\n";
    char user_choice;
    std::cin >> user_choice;
    switch (user_choice)
    {
      case 'p':
      case 'P':
      {
        const auto response = authenticator_api::get_users(authorization);
        response.print();
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

        const auto response =  authenticator_api::add_user(authorization, username, "id", "User", password);
        response.print();
        break;
      }
      case 'r':
      case 'R':
      {
        std::string username;
        std::cout << "username\n";
        std::cin >> username;

        const auto response = authenticator_api::remove_user(authorization, username);
        response.print();
        break;
      }
      case 'l':
      case 'L':
      {
        std::string username;
        std::string password;
        std::cout << "username\n";
        std::cin >> username;
        std::cout << "password\n";
        std::cin >> password;
        const auto response = authenticator_api::login(username, password);
        response.print();
        if (response.authorization > 0)
        {
          authorization = response.authorization;
          std::cout << "Succesefully logged in\n";
        }
        else
        {
          std::cout << "Invalid username or password\n";
        }
        break;
      }
      case 'q':
      case 'Q':
        std::cout << "Goodbye\n";
        run_loop = false;
        break;
      default:
        std::cout << "Invalid input: " << user_choice << '\n';
        break;
    }
  }

  return 0;
}