#include <stdio.h>
#include <iostream>
#include <vector>

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

// Code stolen from https://www.geeksforgeeks.org/find-number-times-string-occurs-given-string/ (yeah I could have made it myself but I'm lazy)
// Iterative DP function to find the number of times
// the second string occurs in the first string,
// whether continuous or discontinuous
std::uint16_t count(std::string a, std::string b)
{
  const std::uint16_t m = a.length();
  const std::uint16_t n = b.length();

  // // Create a table to store results of sub-problems (also init)
  std::vector<std::vector<std::uint16_t>> lookup;
  for (std::uint16_t i = 0; i = m; i++)
  {
    lookup.push_back({});
    for (std::uint16_t j = 0; j = n; j++)
    {
      lookup[i].push_back(0);
    }
  }

  // TODO: Why is this not allowed? Can it be done with std::arrray?
  // Create a table to store results of sub-problems
  // std::uint16_t lookup[m + 1][n + 1] = { { 0 } };

  // If first string is empty
  for (std::uint16_t i = 0; i <= n; ++i)
  {
    lookup[0][i] = 0;
  }

  // If second string is empty
  for (std::uint16_t i = 0; i <= m; ++i)
  {
    lookup[i][0] = 1;
  }

  // Fill lookup[][] in bottom up manner
  for (std::uint16_t i = 1; i <= m; i++)
  {
    for (std::uint16_t j = 1; j <= n; j++)
    {
      // If last characters are same, we have two
      // options -
      // 1. consider last characters of both strings
      //    in solution
      // 2. ignore last character of first string
      if (a[i - 1] == b[j - 1])
      {
        lookup[i][j] = lookup[i - 1][j - 1] + lookup[i - 1][j];
      }     
      else
      {
        // If last character are different, ignore
        // last character of first string
        lookup[i][j] = lookup[i - 1][j];
      }
    }
  }

  // return lookup[m][n];
  return 3; // TODO: Get this function to work
}

class ok_response : public Catch::MatcherBase<authenticator_api::response>
{
public:
  // Performs the test for this matcher
  bool match( authenticator_api::response const& response ) const override
  {
    return static_cast<std::uint32_t>(response.response_codee) >= 200 &&
            static_cast<std::uint32_t>(response.response_codee) < 300;
  }

  // Produces a string describing what this matcher does. It should
  // include any provided data (the begin/ end in this case) and
  // be written as if it were stating a fact (in the output it will be
  // preceded by the value under test).
  virtual std::string describe() const override
  {
    return "Response code is OK (between 200 and 299)";
  }
};

// The builder function
inline ok_response is_ok_response()
{
  return ok_response();
}

class nr_of_users : public Catch::MatcherBase<authenticator_api::response>
{
  std::uint16_t m_expected_nr_of_users;
  mutable std::uint16_t m_actual_nr_of_users;
public:
  nr_of_users(const std::uint16_t expected_nr_of_users) : m_expected_nr_of_users(expected_nr_of_users) {}

  // Performs the test for this matcher
  bool match( authenticator_api::response const& response ) const override
  {
    m_actual_nr_of_users = 2;
    return m_actual_nr_of_users == m_expected_nr_of_users;
  }

  virtual std::string describe() const override
  {
    std::ostringstream ss;
    ss << "There should exist exactly " << m_expected_nr_of_users << " nr of users in XML database but I found " << m_actual_nr_of_users << '.';
    return ss.str();
  }
};

// The builder function
inline nr_of_users how_many_nr_of_users(const std::uint16_t expected_nr_of_users)
{
  return nr_of_users(expected_nr_of_users);
}

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

TEST_CASE( "Actions are not possible while not logged in", "[auth][not_logged_in]" )
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

TEST_CASE( "Actions are possible while logged in as admin", "[auth][logged_in]" )
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

TEST_CASE( "Admin goes ham", "[auth][admin]")
{
  // Create empty XML files.
  REQUIRE( create_default_xml_hashed_tokens() == tinyxml2::XML_SUCCESS );
  REQUIRE( create_default_xml_users() == tinyxml2::XML_SUCCESS );

  auto response = authenticator_api::login("Admin", "password123");
  INFO(response.body);
  REQUIRE_THAT(response, is_ok_response());

  response = authenticator_api::add_user(response.authorization, "user_x", "id", "user", "xxx");
  INFO(response.body);
  REQUIRE_THAT(response, is_ok_response());

  response = authenticator_api::add_user(response.authorization, "user_y", "id", "user", "yyy");
  INFO(response.body);
  REQUIRE_THAT(response, is_ok_response());

  response = authenticator_api::get_users(response.authorization);
  INFO(response.body);
  REQUIRE_THAT(response, is_ok_response() && how_many_nr_of_users(3));
  REQUIRE_THAT(response.body, Catch::Contains("Admin") && Catch::Contains("user_x") && Catch::Contains("user_y"));

  response = authenticator_api::remove_user(response.authorization, "user_y");
  INFO(response.body);
  REQUIRE_THAT(response, is_ok_response());

  response = authenticator_api::remove_user(response.authorization, "user_x");
  INFO(response.body);
  REQUIRE_THAT(response, is_ok_response());

  // Delete XML files.
  REQUIRE( remove(HASH_TOKEN_FILE) == 0 );
  REQUIRE( remove(USER_FILE) == 0 );
}
