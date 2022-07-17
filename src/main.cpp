#include <stdio.h>

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>
#include <ostream>
#include <memory>
#include <stdexcept>

#include "tinyxml2.h"

#define USER_FILE "users.xml"
#define USER_STR "user"
#define USERNAME_STR "username"
#define ID_STR "id"
#define ROLE_STR "role"
#define PASSWORD_STR "password"


// Code from https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
template<typename ... Args>
std::string string_format( const char* format, Args ... args )
{
  int size_s = std::snprintf( nullptr, 0, format, args ... ) + 1; // Extra space for '\0'
  if( size_s <= 0 ) { throw std::runtime_error( "Error during formatting." ); }
  auto size = static_cast<size_t>( size_s );
  std::unique_ptr<char[]> buf( new char[ size ] );
  std::snprintf( buf.get(), size, format, args ... );
  return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

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

  void print() const
  {
    std::cout << "User: " << username <<
                 " Role: " << role <<
                 " Password: "  << password <<
                 " id: " << id << '\n';
  }
};

std::ostream& operator <<(std::ostream& os, const user_t& user)
{
  os << "User: " << user.username <<
        " Role: " << user.role <<
        " Password: "  << user.password <<
        " id: " << user.id;
  return os;
}

[[nodiscard]] std::string user_interal_to_xml_string(const user_t& internal_user)
{
  return string_format("<user id=\"%s\">\n"
                       "\t\t<username>%s</username>\n"
                       "\t\t<role>%s</role>\n"
                       "\t\t<password>%s</password>\n"
                       "\t</user>",
                       internal_user.id.c_str(),
                       internal_user.username.c_str(),
                       internal_user.role.c_str(),
                       internal_user.password.c_str());
}

[[nodiscard]] tinyxml2::XMLElement* user_interal_to_xml_element(tinyxml2::XMLDocument& doc,
                                                                const user_t& user_internal)
{
  tinyxml2::XMLElement* user_element_p = doc.NewElement(USER_STR);
  user_element_p->SetAttribute("id", user_internal.id.c_str());

  tinyxml2::XMLElement* username_element_p = user_element_p->InsertNewChildElement(USERNAME_STR);
  username_element_p->SetText(user_internal.username.c_str());

  tinyxml2::XMLElement* role_element_p = user_element_p->InsertNewChildElement(ROLE_STR);
  role_element_p->SetText(user_internal.role.c_str());

  tinyxml2::XMLElement* password_element_p = user_element_p->InsertNewChildElement(PASSWORD_STR);
  password_element_p->SetText(user_internal.password.c_str());

  return user_element_p;
}

[[nodiscard]] user_t user_xml_element_to_interal(const tinyxml2::XMLElement* user_element_p)
{
  const tinyxml2::XMLElement* username_p = user_element_p->FirstChildElement("username");
  const tinyxml2::XMLElement* role_p = user_element_p->FirstChildElement("role");
  const tinyxml2::XMLElement* password_p = user_element_p->FirstChildElement("password");
  const tinyxml2::XMLAttribute* id_p = user_element_p->FindAttribute("id");
  assert(username_p);
  assert(role_p);
  assert(password_p);
  assert(id_p);

  return user_t(username_p->GetText(), id_p->Value(), role_p->GetText(), password_p->GetText());
}

[[nodiscard]] tinyxml2::XMLElement* load_root_element(tinyxml2::XMLDocument& doc,
                                                      tinyxml2::XMLError& result)
{
  result = tinyxml2::XML_SUCCESS;
  tinyxml2::XMLElement* root_p = doc.FirstChildElement("users");
  if (root_p == nullptr)
  {
    result = tinyxml2::XML_NO_ATTRIBUTE;
    std::cerr << __FILE__ << ':' << __LINE__ << " ERROR: " << result << '\n';
  }
  return root_p;
}

[[nodiscard]] tinyxml2::XMLError load_all_users(tinyxml2::XMLDocument& doc,
                                                std::vector<user_t>& users)
{
  tinyxml2::XMLError result;
  tinyxml2::XMLElement* root_p = load_root_element(doc, result);
  if (result != tinyxml2::XML_SUCCESS)
  {
    return result;
  }
  assert(root_p);
  if (result != tinyxml2::XML_SUCCESS)
  {
    return result;
  }
  tinyxml2::XMLElement* user_p = root_p->FirstChildElement("user");
  assert(user_p);

  while (user_p != nullptr)
  {
    users.push_back(user_xml_element_to_interal(user_p));
    user_p = user_p->NextSiblingElement();
  }

  return tinyxml2::XML_SUCCESS;
}

[[nodiscard]] tinyxml2::XMLError add_user(tinyxml2::XMLDocument& doc,
                                          std::vector<user_t>& users)
{
  std::string username;
  std::cout << "username\n";

  std::cin >> username;  
  auto p = std::find_if(users.begin(), users.end(), [&username] (const user_t& user)
    {
      return user.username == username;
    });

  if (p != users.end())
  {
    std::cout << "User '" << username << "' already exists.\n";
    return tinyxml2::XML_SUCCESS;
  }

  std::cout << "password\n";
  std::string password;
  std::cin >> password;

  user_t new_user = user_t(username, "id", "User", password);
  users.push_back(new_user);
  auto user_xml_element_p = user_interal_to_xml_element(doc, new_user);

  tinyxml2::XMLError result;
  tinyxml2::XMLElement* root_p = load_root_element(doc, result);

  if (result != tinyxml2::XML_SUCCESS)
  {
    return result;
  }

  root_p->LinkEndChild(user_xml_element_p);
  return  tinyxml2::XML_SUCCESS;
}

int main()
{
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLError result = doc.LoadFile(USER_FILE);
  if (result != tinyxml2::XML_SUCCESS)
  {
    std::cerr << __FILE__ << ':' << __LINE__ << " ERROR: " << result << '\n';
    return result;
  }

  std::vector<user_t> users;
  result = load_all_users(doc, users);
  if (result != tinyxml2::XML_SUCCESS)
  {
    return static_cast<int>(result);
  }
  
  std::cout << "What would you like to do? Print users (P) or add user (A)" << std::endl;
  char user_choice;
  std::cin >> user_choice;

  switch (user_choice)
  {
    case 'p':
    case 'P':
      for (user_t user : users)
      {
        user.print();
      }
      break;
    case 'a':
    case 'A':
    {
      result = add_user(doc, users);
      if (result != tinyxml2::XML_SUCCESS)
      {
        return static_cast<int>(result);
      }

      doc.SaveFile(USER_FILE);
      break;
    }
    default:
      std::cout << "Invalid input: " << user_choice << '\n';
      break;
  }

  return 0;
}