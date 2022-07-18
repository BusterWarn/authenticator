#pragma once

#include <string>
#include <ostream>
#include <vector>

namespace authenticator_api
{

struct [[nodiscard]] user_t
{
  std::string username;
  std::string id;
  std::string role;
  std::string password;

  user_t(const std::string username_input,
         const std::string id_input,
         const std::string role_input,
         const std::string password_input)
         : username(username_input),
           id(id_input),
           role(role_input),
           password(password_input)
  {
  }

  void print() const;
};

std::ostream& operator <<(std::ostream& os, const user_t& user);

void add_user(const std::string& username,
              const std::string& id,
              const std::string& role,
              const std::string& password);

void remove_user(const std::string& username);

std::vector<user_t> get_users();

}