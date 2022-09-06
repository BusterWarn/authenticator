#include <iostream>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "authenticator_api.h"


#ifndef HASH_TOKEN_FILE
#define HASH_TOKEN_FILE "../xml/hashed_tokens.xml"
#endif

unsigned int Factorial( unsigned int number ) {
    return number <= 1 ? number : Factorial(number-1)*number;
}



TEST_CASE( "Actions are not possible while not logged in", "[auth]" )
{
  SECTION( "Get users" )
  {
    const auto response = authenticator_api::get_users(10);
    std::cout << "Actual: " << (int)response.response_codee << " Expected: " << (int) authenticator_api::response_code::UNAUTHORIZED_401 << "\n";
    REQUIRE( response.response_codee == authenticator_api::response_code::UNAUTHORIZED_401 );
  }

  SECTION( "Add user" )
  {
    const auto response = authenticator_api::add_user(10, "user_x", "id", "admin", "password123");
    REQUIRE( response.response_codee == authenticator_api::response_code::UNAUTHORIZED_401 );
  }

  SECTION( "Remove user" )
  {
    const auto response = authenticator_api::remove_user(10, "user_x");
    REQUIRE( response.response_codee == authenticator_api::response_code::UNAUTHORIZED_401 );
  }
}

TEST_CASE( "Actions are possible while logged in as admin", "[auth]" )
{
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
    REQUIRE( response.response_codee == authenticator_api::response_code::OK_200 );
  }

  SECTION( "Remove user" )
  {
    response = authenticator_api::remove_user(response.authorization, "user_x");
    INFO(response.body);
    REQUIRE( response.response_codee == authenticator_api::response_code::OK_200 );
  }
}
