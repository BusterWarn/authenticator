#include <stdio.h>
#include <iostream>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "authenticator_api.h"
#include "tinyxml2.h"

#ifndef HASH_TOKEN_FILE
#define HASH_TOKEN_FILE "../build/hashed_tokens.xml"
#endif

#ifndef USER_FILE
#define USER_FILE "../build/users.xml"
#endif

tinyxml2::XMLError create_default_xml_users()
{
  const char* xml =
    "<?xml version=\"1.0\"?>"
    "<users>"
    "<user id=\"admin\">"
    "    <username>Admin</username>"
    "    <role>Admin</role>"
    "    <password>password123</password>"
    "</user>"
    "</users>";
  tinyxml2::XMLDocument doc;
  tinyxml2::XMLError error = doc.Parse(xml);
  if (error != tinyxml2::XML_SUCCESS)
  {
    return error;
  }

  return doc.SaveFile(USER_FILE);
}

tinyxml2::XMLError create_default_xml_hashed_tokens()
{
  const char* xml =
    "<?xml version=\"1.0\"?>"
    "<hashed_tokens />";
  tinyxml2::XMLDocument doc;
  tinyxml2::XMLError error = doc.Parse(xml);
  if (error != tinyxml2::XML_SUCCESS)
  {
    std::cout << __FILE__ << ':' << __LINE__ << "ERROR while parsing xml: " << error << "\n\n";
    return error;
  }

  return doc.SaveFile(HASH_TOKEN_FILE);
}

TEST_CASE( "Actions are not possible while not logged in", "[auth][stupid]" )
{
  // Create empty XML files.
  REQUIRE( create_default_xml_hashed_tokens() == tinyxml2::XML_SUCCESS );
  REQUIRE( create_default_xml_users() == tinyxml2::XML_SUCCESS );

  SECTION( "Get users" )
  {
    const auto response = authenticator_api::get_users(10);
    INFO(response.body);
    REQUIRE( response.response_codee == authenticator_api::response_code::UNAUTHORIZED_401 );
  }

  SECTION( "Add user" )
  {
    const auto response = authenticator_api::add_user(10, "user_x", "id", "admin", "password123");
    INFO(response.body);
    REQUIRE( response.response_codee == authenticator_api::response_code::UNAUTHORIZED_401 );
  }

  SECTION( "Remove user" )
  {
    const auto response = authenticator_api::remove_user(10, "user_x");
    INFO(response.body);
    REQUIRE( response.response_codee == authenticator_api::response_code::UNAUTHORIZED_401 );
  }

  // Delete XML files.
  REQUIRE( remove(HASH_TOKEN_FILE) == 0 );
  REQUIRE( remove(USER_FILE) == 0 );
}

TEST_CASE( "Actions are possible while logged in as admin", "[auth]" )
{
  // Create empty XML files.
  REQUIRE( create_default_xml_hashed_tokens() == tinyxml2::XML_SUCCESS );
  REQUIRE( create_default_xml_users() == tinyxml2::XML_SUCCESS );

  auto response = authenticator_api::login("Admin", "password123");
  INFO(response.body);
  REQUIRE( response.response_codee == authenticator_api::response_code::OK_200 );

  SECTION( "Get users" )
  {
    response = authenticator_api::get_users(response.authorization);
    INFO(response.body);
    REQUIRE( response.response_codee == authenticator_api::response_code::OK_200 );
  }

  SECTION( "Add user" )
  {
    response = authenticator_api::add_user(response.authorization, "user_x", "id", "user", "password123");
    INFO(response.body);
    REQUIRE( response.response_codee == authenticator_api::response_code::CREATED_201 );
  }

  SECTION( "Remove non existing user" )
  {
    response = authenticator_api::remove_user(response.authorization, "user_x");
    INFO(response.body);
    // File should be empty so no user to remove.
    REQUIRE( response.response_codee == authenticator_api::response_code::NOT_FOUND_404 );
  }

  SECTION( "Remove existing user" )
  {
    response = authenticator_api::add_user(response.authorization, "user_x", "id", "user", "password123");
    INFO(response.body);
    REQUIRE( response.response_codee == authenticator_api::response_code::CREATED_201 );

    response = authenticator_api::remove_user(response.authorization, "user_x");
    INFO(response.body);
    REQUIRE( response.response_codee == authenticator_api::response_code::OK_200 );
  }

  // Delete XML files.
  REQUIRE( remove(HASH_TOKEN_FILE) == 0 );
  REQUIRE( remove(USER_FILE) == 0 );
}
