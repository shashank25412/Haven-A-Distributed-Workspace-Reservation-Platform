#include "haven/infrastructure/cache/redis/resource_detail_cache_key.hpp"

#include <cctype>
#include <string_view>

namespace haven::infrastructure::cache::redis {
namespace {
std::string encode(std::string_view value) {
    constexpr char hex[] = "0123456789ABCDEF";
    std::string encoded;
    for (const unsigned char character : value) {
        if (std::isalnum(character) != 0 || character == '-' || character == '_' ||
            character == '.') {
            encoded.push_back(static_cast<char>(character));
        } else {
            encoded.push_back('%');
            encoded.push_back(hex[character >> 4U]);
            encoded.push_back(hex[character & 0x0FU]);
        }
    }
    return encoded;
}
}  // namespace

std::string resource_detail_cache_key(const haven::domain::OrganizationId& organization_id,
                                      const haven::domain::ResourceId& resource_id) {
    return "haven:v1:org:" + encode(organization_id.value()) +
           ":resource:" + encode(resource_id.value());
}
}  // namespace haven::infrastructure::cache::redis
