#include "authenticator_api.h"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>
#include <ostream>
#include <sstream>
#include <memory>
#include <chrono>

#include "tinyxml2.h"

#ifndef HASH_TOKEN_FILE
#define HASH_TOKEN_FILE "hashed_tokens.xml"
#endif

#ifndef USER_FILE
#define USER_FILE "users.xml"
#endif

#define TOKEN_ROOT_ELEMENT_STR "hashed_tokens"
#define TOKEN_ELEMENT_STR "hashed_token"
#define TOKEN_TIME_ATTRIBUTE_STR "time"

#define USER_STR "user"
#define USERNAME_STR "username"
#define ID_STR "id"
#define ROLE_STR "role"
#define PASSWORD_STR "password"

constexpr std::uint64_t TOKEN_EXPIRATION_TIME = 60; // 60 seconds

namespace authenticator_api
{
  /**
   * djb2 hash function. No idea what it does but it works!
   */
  std::uint64_t stupid_hash(const char *str)
  {
      std::uint64_t hash = 5381;
      int c;
      while ((c = *str++))
      {
          hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
      }
      if (hash == 0)
      {
        hash++;
      }

      return hash;
  }

  // Code from https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
  template<typename ... Args>
  std::unique_ptr<char[]> c_string_format( const char* format, Args ... args )
  {
    int size_s = std::snprintf( nullptr, 0, format, args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ) { throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format, args ... );
    return buf;
  }

  template<typename ... Args>
  std::string string_format( const char* format, Args ... args )
  {
    int size_s = std::snprintf( nullptr, 0, format, args ... );
    if( size_s <= 0 ) { throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf = c_string_format(format, args ...);
    return std::string( buf.get(), buf.get() + size ); // We don't want the '\0' inside
  }
  
  std::uint64_t get_current_time()
  {
    const auto system_clock = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(system_clock.time_since_epoch()).count();
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
                        "\t<username>%s</username>\n"
                        "\t<role>%s</role>\n"
                        "\t<password>%s</password>\n"
                        "</user>",
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
    const tinyxml2::XMLElement* username_p = user_element_p->FirstChildElement(USERNAME_STR);
    const tinyxml2::XMLElement* role_p = user_element_p->FirstChildElement(ROLE_STR);
    const tinyxml2::XMLElement* password_p = user_element_p->FirstChildElement(PASSWORD_STR);
    const tinyxml2::XMLAttribute* id_p = user_element_p->FindAttribute(ID_STR);
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
      std::cerr << __FILE__ << ':' << __LINE__ << " ERROR: " << result << '\n';
      return result;
    }
    assert(root_p);
    tinyxml2::XMLElement* user_p = root_p->FirstChildElement(USER_STR);
    assert(user_p);

    while (user_p != nullptr)
    {
      users.push_back(user_xml_element_to_interal(user_p));
      user_p = user_p->NextSiblingElement();
    }

    return tinyxml2::XML_SUCCESS;
  }

  [[nodiscard]] tinyxml2::XMLElement* get_user(tinyxml2::XMLDocument& doc,
                                               const std::string& username)
  {
    tinyxml2::XMLError result;
    tinyxml2::XMLElement* root_p = load_root_element(doc, result);
    if (result != tinyxml2::XML_SUCCESS)
    {
      std::cerr << __FILE__ << ':' << __LINE__ << " ERROR: Could not load root element (" << result << ")\n";
      return nullptr;
    }
    assert(root_p);
    tinyxml2::XMLElement* user_p = root_p->FirstChildElement(USER_STR);
    assert(user_p);

    while (user_p != nullptr)
    {
      const tinyxml2::XMLElement* username_p = user_p->FirstChildElement(USERNAME_STR);
      if (username_p->GetText() == username)
      {
        return user_p;
      }
      user_p = user_p->NextSiblingElement();
    }

    return nullptr;
  }

  [[nodiscard]] tinyxml2::XMLElement* get_token_element(tinyxml2::XMLDocument& token_doc,
                                                        const std::uint64_t hashed_token)
  {
    auto token_root_p =  token_doc.FirstChildElement(TOKEN_ROOT_ELEMENT_STR);
    auto token_element_p = token_root_p->FirstChildElement();
    while (token_element_p != nullptr)
    {
      try
      {
        std::uint64_t stored_token = std::stoul(token_element_p->GetText());
        if (stored_token == hashed_token)
        {
          return token_element_p;
        }
      }
      catch(const std::exception& e)
      {
        std::cerr << __FILE__ << ':' << __LINE__ << "ERROR: " << e.what()
                  << " invalid hash: " << token_element_p->GetText() << '\n';
        return nullptr;
      }
      
      token_element_p = token_element_p->NextSiblingElement();
    }

    return nullptr;
  }

  [[nodiscard]] bool has_token_expired(tinyxml2::XMLDocument& token_doc,
                                       const std::uint64_t hashed_token)
  {
    auto token_element_p = get_token_element(token_doc, hashed_token);
    if (!token_element_p)
    {
      return true;
    }

    try
    {
      std::uint64_t token_timestamp = std::stoul(token_element_p->Attribute(TOKEN_TIME_ATTRIBUTE_STR));
      std::uint64_t current_time = get_current_time(); 
      assert(current_time >= token_timestamp);
      std::uint64_t token_passed_time = current_time - token_timestamp;
      if (token_passed_time > TOKEN_EXPIRATION_TIME)
      {
        return true;
      }
    }
    catch(const std::exception& e)
    {
      std::cerr << __FILE__ << ':' << __LINE__ << "ERROR: " << e.what()
                << " invalid hash: " << token_element_p->GetText() << '\n';
      return true;
    }
    return false;
  }

  [[nodiscard]] bool has_token_expired(const std::uint64_t hashed_token)
  {
    tinyxml2::XMLDocument token_doc;
    auto result = token_doc.LoadFile(HASH_TOKEN_FILE);

    if (result != tinyxml2::XML_SUCCESS)
    {
      return false;
    }
    
    return has_token_expired(token_doc, hashed_token);
  }

  [[nodiscard]] tinyxml2::XMLError store_token(const std::uint64_t hashed_token)
  {
    tinyxml2::XMLDocument token_doc;
    auto result = token_doc.LoadFile(HASH_TOKEN_FILE);
    if (result != tinyxml2::XML_SUCCESS)
    {
      std::cerr << __FILE__ << ":" << __LINE__ << " ERROR: " << result << '\n';
      return result;
    }
    const std::uint64_t current_time = get_current_time();

    auto token_element_p = get_token_element(token_doc, hashed_token);
    if (token_element_p)
    {
      token_element_p->SetAttribute(TOKEN_TIME_ATTRIBUTE_STR, current_time);
    }
    else
    {
      token_element_p = token_doc.NewElement(TOKEN_ELEMENT_STR);
      assert(token_element_p);
      token_element_p->SetAttribute(TOKEN_TIME_ATTRIBUTE_STR, current_time);
      token_element_p->SetText(hashed_token);

      auto token_root_p =  token_doc.FirstChildElement(TOKEN_ROOT_ELEMENT_STR);
      token_root_p->LinkEndChild(token_element_p);
    }

    token_doc.SaveFile(HASH_TOKEN_FILE);
    return token_doc.ErrorID();
  }

  void response::print() const
  {
    std::cout << "Response code: " << static_cast<std::uint32_t>(response_codee) << '\n'
              << "Authorization: " << authorization << '\n'
              << "Body:          { " << body << " }\n";
  }

  const response add_user(const std::uint64_t authorization_token,
                          const std::string& username,
                          const std::string& id,
                          const std::string& role,
                          const std::string& password)
  {
    if (has_token_expired(authorization_token))
    {
      return response(response_code::UNAUTHORIZED_401,
                      authorization_token,
                      string_format("%s:%u Authorization token has expired.", __FILE__, __LINE__));
    }
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError result = doc.LoadFile(USER_FILE);
    if (result != tinyxml2::XML_SUCCESS)
    {
      return response(response_code::INTERNAL_SERVER_ERROR_500,
                      0,
                      string_format("%s:%u ERROR: %u", __FILE__, __LINE__, result));
    }

    std::vector<user_t> users;
    result = load_all_users(doc, users);
    if (result != tinyxml2::XML_SUCCESS)
    {
      return response(response_code::INTERNAL_SERVER_ERROR_500,
                      authorization_token,
                      string_format("%s:%u ERROR: %u", __FILE__, __LINE__, result));
    }

    auto user_it = std::find_if(users.begin(), users.end(), [&username] (const user_t& user)
      {
        return user.username == username;
      });

    if (user_it != users.end())
    {
      return response(response_code::BAD_REQUEST_400,
                      authorization_token,
                      string_format("%s:%u User '%s' already exists.", __FILE__, __LINE__, username.c_str()));
    }

    user_t new_user = user_t(username, id, role, password);
    users.push_back(new_user);
    auto user_xml_element_p = user_interal_to_xml_element(doc, new_user);

    tinyxml2::XMLElement* root_p = load_root_element(doc, result);

    if (result != tinyxml2::XML_SUCCESS)
    {
      return response(response_code::INTERNAL_SERVER_ERROR_500,
                      authorization_token,
                      string_format("%s:%u ERROR: %u", __FILE__, __LINE__, result));
    }

    root_p->LinkEndChild(user_xml_element_p);
    doc.SaveFile(USER_FILE);
    
    return response(response_code::CREATED_201,
                    authorization_token,
                    string_format("User Created:\n", user_interal_to_xml_string(new_user).c_str()));
  }

  const response remove_user(const std::uint64_t authorization_token,
                             const std::string& username)
  {
    if (has_token_expired(authorization_token))
    {
      return response(response_code::UNAUTHORIZED_401,
                      authorization_token,
                      string_format("%s:%u Authorization token has expired.", __FILE__, __LINE__));
    }
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError result = doc.LoadFile(USER_FILE);
    if (result != tinyxml2::XML_SUCCESS)
    {
      return response(response_code::INTERNAL_SERVER_ERROR_500,
                      authorization_token,
                      string_format("%s:%u ERROR: %u", __FILE__, __LINE__, result));
    }

    std::vector<user_t> users;
    result = load_all_users(doc, users);
    if (result != tinyxml2::XML_SUCCESS)
    {
      return response(response_code::INTERNAL_SERVER_ERROR_500,
                      authorization_token,
                      string_format("%s:%u ERROR: %u", __FILE__, __LINE__, result));
    }

    auto user_it = std::find_if(users.begin(), users.end(), [&username] (const user_t& user)
      {
        return user.username == username;
      });

    if (user_it == users.end())
    {
      return response(response_code::NOT_FOUND_404,
                      authorization_token,
                      string_format("%s:%u No user exists with username '%s'", __FILE__, __LINE__, username.c_str()));
    }

    auto user_xml_element_p = get_user(doc, (*user_it).username);
    if (!user_xml_element_p)
    {
      return response(response_code::INTERNAL_SERVER_ERROR_500,
                      authorization_token,
                      string_format("%s:%u ERROR: Could not fetch user from XML doc", __FILE__, __LINE__));
    }

    tinyxml2::XMLElement* root_p = load_root_element(doc, result);
    if (result != tinyxml2::XML_SUCCESS)
    {
      return response(response_code::INTERNAL_SERVER_ERROR_500,
                      authorization_token,
                      string_format("%s:%u ERROR: %u", __FILE__, __LINE__, result));
    }
    assert(root_p);

    root_p->DeleteChild(user_xml_element_p);
    doc.SaveFile(USER_FILE);
    
    return response(response_code::OK_200,
                    authorization_token,
                    string_format("User deleted:\n", user_interal_to_xml_string(*user_it).c_str()));
  }

  const response get_users(const std::uint64_t authorization_token)
  {
    if (has_token_expired(authorization_token))
    {
      return response(response_code::UNAUTHORIZED_401,
                      authorization_token,
                      string_format("%s:%u Authorization token has expired.", __FILE__, __LINE__));
    }
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError result = doc.LoadFile(USER_FILE);
    if (result != tinyxml2::XML_SUCCESS)
    {
      return response(response_code::INTERNAL_SERVER_ERROR_500,
                      authorization_token,
                      string_format("%s:%u ERROR: %u", __FILE__, __LINE__, result));
    }

    std::vector<user_t> users;
    result = load_all_users(doc, users);
    if (result != tinyxml2::XML_SUCCESS)
    {
      return response(response_code::INTERNAL_SERVER_ERROR_500,
                      authorization_token,
                      string_format("%s:%u ERROR: %u", __FILE__, __LINE__, result));
    }

    std::ostringstream ss(std::ostringstream::ate);
    ss << "users: { ";
    for (std::uint32_t i = 0; i < users.size(); i++)
    {
      user_t user = users.at(i);
      ss << "{ username: " << user.username << ", role: " << user.role << " } " << (i == users.size() - 1 ? "" : ", ");
    }
    ss << "}";

    return response(response_code::OK_200,
                    authorization_token,
                    ss.str());;
  }

  const response login(const std::string& username,
                       const std::string& password)
  {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError result = doc.LoadFile(USER_FILE);

    if (result != tinyxml2::XML_SUCCESS)
    {
      return response(response_code::INTERNAL_SERVER_ERROR_500,
                      0,
                      string_format("%s:%u ERROR: %u - Could not load file: %s", __FILE__, __LINE__, result, USER_FILE));
    }

    auto user_xml_element_p = get_user(doc, username);

    if (!user_xml_element_p)
    {
      return response(response_code::NOT_FOUND_404,
                      0,
                      string_format("%s:%u No user exists with username '%s'", __FILE__, __LINE__, username.c_str()));
    }

    auto user_interal = user_xml_element_to_interal(user_xml_element_p);
    if (user_interal.password != password)
    {
      return response(response_code::UNAUTHORIZED_401,
                      0,
                      string_format("%s:%u Invalid Password '%s'", __FILE__, __LINE__, password.c_str()));
    }

    std::uint64_t hash = stupid_hash(c_string_format("%s%s", username.c_str(), password.c_str()).get());
    result = store_token(hash);
    if (result != tinyxml2::XML_SUCCESS)
    {
      return response(response_code::INTERNAL_SERVER_ERROR_500,
                      0,
                      string_format("%s:%u ERROR: %u", __FILE__, __LINE__, result));
    }

    return response(response_code::OK_200,
                    hash,
                    "");
  }

}