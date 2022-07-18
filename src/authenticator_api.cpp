#include "authenticator_api.h"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>
#include <ostream>
#include <memory>
#include <stdexcept>
#include <chrono>

#include "tinyxml2.h"

#define HASH_LOGIN_FILE "hashed_logins.xml"
#define HASH_ROOT_ELEMENT_STR "hashes"
#define HASH_ELEMENT_STR "hashed_login"
#define HASH_TIME_ATTRIBUTE_STR "time"

#define USER_FILE "users.xml"
#define USER_STR "user"
#define USERNAME_STR "username"
#define ID_STR "id"
#define ROLE_STR "role"
#define PASSWORD_STR "password"

namespace authenticator_api
{
  /**
   * djb2 hash function. No idea what it does but it works!
   */
  std::uint64_t simple_hash(const char *str)
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

  void user_t::print() const
  {
    std::cout << "User: " << username <<
                 " Role: " << role <<
                 " Password: "  << password <<
                 " id: " << id << '\n';
  }

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
      std::cerr << __FILE__ << ':' << __LINE__ << " ERROR: " << result << '\n';
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

  [[nodiscard]] tinyxml2::XMLElement* get_hash_element(tinyxml2::XMLDocument& hash_doc,
                                                       const std::uint64_t hash)
  {
    const std::uint64_t current_time = get_current_time();

    auto hash_root_p =  hash_doc.FirstChildElement(HASH_ROOT_ELEMENT_STR);
    auto hash_element_p = hash_root_p->FirstChildElement();
    while (hash_element_p != nullptr)
    {
      try
      {
        std::cerr << "KOm hit! :D\n";
        std::uint64_t hash_value = std::stoul(hash_element_p->GetText());
        if (hash_value == hash)
        {
          return hash_element_p;
        }
      }
      catch(const std::exception& e)
      {
        std::cerr << __FILE__ << ':' << __LINE__ << "ERROR: " << e.what()
                  << " invalid hash: " << hash_element_p->GetText() << '\n';
        return nullptr;
      }
      
      hash_element_p = hash_element_p->NextSiblingElement();
    }

    std::cerr << "RETURN NULLPTR" << std::endl;
    return nullptr;
  }

  [[nodiscard]] tinyxml2::XMLError store_hash(std::uint64_t hash)
  {
    tinyxml2::XMLDocument hash_doc;
    auto result = hash_doc.LoadFile(HASH_LOGIN_FILE);
    if (result != tinyxml2::XML_SUCCESS)
    {
      std::cerr << __FILE__ << ":" << __LINE__ << " ERROR: " << result << '\n';
      return result;
    }
    const std::uint64_t current_time = get_current_time();

    auto hash_element_p = get_hash_element(hash_doc, hash);
    if (hash_element_p)
    {
      hash_element_p->SetAttribute(HASH_TIME_ATTRIBUTE_STR, current_time);
    }
    else
    {
      hash_element_p = hash_doc.NewElement(HASH_ELEMENT_STR);
      assert(hash_element_p);
      hash_element_p->SetAttribute(HASH_TIME_ATTRIBUTE_STR, current_time);
      hash_element_p->SetText(hash);

      auto hash_root_p =  hash_doc.FirstChildElement(HASH_ROOT_ELEMENT_STR);
      hash_root_p->LinkEndChild(hash_element_p);
    }

    hash_doc.SaveFile(HASH_LOGIN_FILE);
    return hash_doc.ErrorID();
  }

  void add_user(const std::string& username,
                const std::string& id,
                const std::string& role,
                const std::string& password)
  {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError result = doc.LoadFile(USER_FILE);
    if (result != tinyxml2::XML_SUCCESS)
    {
      std::cerr << __FILE__ << ':' << __LINE__ << " ERROR: " << result << '\n';
      return;
    }

    std::vector<user_t> users;
    result = load_all_users(doc, users);
    if (result != tinyxml2::XML_SUCCESS)
    {
      return;
    }

    auto user_it = std::find_if(users.begin(), users.end(), [&username] (const user_t& user)
      {
        return user.username == username;
      });

    if (user_it != users.end())
    {
      std::cout << "User '" << username << "' already exists.\n";
      return;
    }

    user_t new_user = user_t(username, "id", "User", password);
    users.push_back(new_user);
    auto user_xml_element_p = user_interal_to_xml_element(doc, new_user);

    tinyxml2::XMLElement* root_p = load_root_element(doc, result);

    if (result != tinyxml2::XML_SUCCESS)
    {
      std::cerr << __FILE__ << ':' << __LINE__ << " ERROR: " << result << '\n';
      return;
    }

    std::cout << "User added:\n" << user_interal_to_xml_string(new_user) << '\n';
    root_p->LinkEndChild(user_xml_element_p);
    
    doc.SaveFile(USER_FILE);
  }

  void remove_user(const std::string& username)
  {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError result = doc.LoadFile(USER_FILE);
    if (result != tinyxml2::XML_SUCCESS)
    {
      std::cerr << __FILE__ << ':' << __LINE__ << " ERROR: " << result << '\n';
      return;
    }

    std::vector<user_t> users;
    result = load_all_users(doc, users);
    if (result != tinyxml2::XML_SUCCESS)
    {
      return;
    }

    auto user_it = std::find_if(users.begin(), users.end(), [&username] (const user_t& user)
      {
        return user.username == username;
      });

    if (user_it == users.end())
    {
      std::cout << "User '" << username << "' does not exist.\n";
      return;
    }

    auto user_xml_element_p = get_user(doc, (*user_it).username);
    if (!user_xml_element_p)
    {
      std::cerr << __FILE__ << ':' << __LINE__ << " ERROR: Could not fetch user from XML doc" << '\n';
      return;
    }

    tinyxml2::XMLElement* root_p = load_root_element(doc, result);
    if (result != tinyxml2::XML_SUCCESS)
    {
      std::cerr << __FILE__ << ':' << __LINE__ << " ERROR: " << result << '\n';
      return;
    }
    assert(root_p);

    root_p->DeleteChild(user_xml_element_p);
    std::cout << "User deleted:\n" << user_interal_to_xml_string(*user_it) << '\n';
    doc.SaveFile(USER_FILE);
  }

  std::vector<user_t> get_users()
  {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError result = doc.LoadFile(USER_FILE);
    if (result != tinyxml2::XML_SUCCESS)
    {
      std::cerr << __FILE__ << ':' << __LINE__ << " ERROR: " << result << '\n';
      return {};
    }

    std::vector<user_t> users;
    result = load_all_users(doc, users);
    if (result != tinyxml2::XML_SUCCESS)
    {
      return {};
    }

    return users;
  }

  std::uint64_t login(const std::string& username,
                      const std::string& password)
  {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError result = doc.LoadFile(USER_FILE);
    if (result != tinyxml2::XML_SUCCESS)
    {
      std::cerr << __FILE__ << ':' << __LINE__ << " ERROR: " << result << '\n';
      return 0;
    }

    auto user_xml_element_p = get_user(doc, username);

    if (!user_xml_element_p)
    {
      return 0;
    }

    auto user_interal = user_xml_element_to_interal(user_xml_element_p);
    if (user_interal.password != password)
    {
      return 0;
    }

    std::uint64_t hash = simple_hash(c_string_format("%s%s", username.c_str(), password.c_str()).get());
    result = store_hash(hash);
    if (result != tinyxml2::XML_SUCCESS)
    {
      return 0;
    }

    return hash;
  }

}