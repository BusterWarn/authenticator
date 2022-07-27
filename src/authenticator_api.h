#pragma once

#include <string>

namespace authenticator_api
{

    enum class response_code
    {
      OK_200 = 200,
      CREATED_201 = 201,
      BAD_REQUEST_400 = 400,
      UNAUTHORIZED_401 = 401,
      NOT_FOUND_404 = 404,
      INTERNAL_SERVER_ERROR_500 = 500
    };

    struct [[nodiscard]] response
    {
        response_code response_codee;
        std::uint64_t authorization; 

        std::string body;

        response(const response_code input_response_code,
                 const std::uint64_t input_authorization,
                 const std::string& input_body)
        : response_codee(input_response_code),
          authorization(input_authorization),
          body(input_body)
        {
        }

        void print() const;
    };

  const response add_user(const std::uint64_t authorization_token,
                          const std::string& username,
                          const std::string& id,
                          const std::string& role,
                          const std::string& password);

  const response remove_user(const std::uint64_t authorization_token,
                             const std::string& username);

  const response get_users(const std::uint64_t authorization_token);

  const response login(const std::string& username,
                       const std::string& password);

}