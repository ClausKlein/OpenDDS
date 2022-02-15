/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h"

#include <DCPS/NetworkAddress.h>

#include <DCPS/Hash.h>
#include <DCPS/LogAddr.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

NetworkAddress::NetworkAddress()
{
  // We want to take care here to make sure that our NetworkAddress can be used with memcmp
  memset(&inet_addr_, 0, sizeof (inet_addr_));

  // The following is required to achieve equivalence between default-constructed NetworkAddress and ACE_INET_Addr
#if defined (ACE_HAS_IPV6)
#  if defined (ACE_USES_IPV4_IPV6_MIGRATION)
  if (ACE::ipv6_enabled()) {
    inet_addr_.in6_.sin6_family = AF_INET6;
#    ifdef ACE_HAS_SOCKADDR_IN_SIN_LEN
    inet_addr_.in6_.sin6_len = sizeof (inet_addr_);
#    endif /* ACE_HAS_SOCKADDR_IN_SIN_LEN */
  } else {
    inet_addr_.in4_.sin_family = AF_INET;
#    ifdef ACE_HAS_SOCKADDR_IN_SIN_LEN
    inet_addr_.in4_.sin_len = sizeof (inet_addr_);
#    endif /* ACE_HAS_SOCKADDR_IN_SIN_LEN */
  }

#  else /* ACE_USES_IPV4_IPV6_MIGRATION */
  inet_addr_.in6_.sin6_family = AF_INET6;
#    ifdef ACE_HAS_SOCKADDR_IN_SIN_LEN
  inet_addr_.in6_.sin6_len = sizeof (inet_addr_);
#    endif /* ACE_HAS_SOCKADDR_IN_SIN_LEN */
#  endif /* ACE_USES_IPV4_IPV6_MIGRATION */

#else /* ACE_HAS_IPV6 */
  inet_addr_.in4_.sin_family = AF_INET;
#  ifdef ACE_HAS_SOCKADDR_IN_SIN_LEN
  inet_addr_.in4_.sin_len = sizeof (inet_addr_);
#  endif /* ACE_HAS_SOCKADDR_IN_SIN_LEN */
#endif /* ACE_HAS_IPV6 */
}

NetworkAddress::NetworkAddress(const NetworkAddress& val)
{
  *this = val;
}

NetworkAddress::NetworkAddress(const char* str)
{
  *this = ACE_INET_Addr(str);
}

NetworkAddress::NetworkAddress(uint16_t port, const char* str)
{
  *this = ACE_INET_Addr(port, str);
}

#if defined (ACE_HAS_WCHAR)

NetworkAddress::NetworkAddress(const wchar_t* str)
{
  *this = ACE_INET_Addr(str);
}

NetworkAddress::NetworkAddress(uint16_t port, const wchar_t* str)
{
  *this = ACE_INET_Addr(port, str);
}

#endif

NetworkAddress::NetworkAddress(const ACE_INET_Addr& addr)
{
  // We want to take care here to make sure that our NetworkAddress can be used with memcmp
  memset(&inet_addr_, 0, sizeof (inet_addr_));

  const ip46& in = *reinterpret_cast<ip46*>(addr.get_addr());
  if (in.in4_.sin_family == AF_INET) {
    inet_addr_.in4_.sin_family = in.in4_.sin_family;
    inet_addr_.in4_.sin_port = in.in4_.sin_port;
#ifdef ACE_HAS_SOCKADDR_IN_SIN_LEN
    inet_addr_.in4_.sin_len = in.in4_.sin_len;
#endif
    inet_addr_.in4_.sin_addr = in.in4_.sin_addr;
#if defined (ACE_HAS_IPV6)
  } else if (in.in6_.sin6_family == AF_INET6) {
    inet_addr_.in6_.sin6_family = in.in6_.sin6_family;
    inet_addr_.in6_.sin6_port = in.in6_.sin6_port;
    inet_addr_.in6_.sin6_flowinfo = in.in6_.sin6_flowinfo;
#ifdef ACE_HAS_SOCKADDR_IN_SIN_LEN
    inet_addr_.in6_.sin6_len = in.in6_.sin6_len;
#endif
    inet_addr_.in6_.sin6_addr = in.in6_.sin6_addr;
    inet_addr_.in6_.sin6_scope_id = in.in6_.sin6_scope_id;
#endif
  }
}

NetworkAddress& NetworkAddress::operator=(const NetworkAddress& rhs)
{
  if (&rhs != this) {
    memcpy(&inet_addr_, &rhs.inet_addr_, sizeof (inet_addr_));
  }
  return *this;
}

NetworkAddress& NetworkAddress::operator=(const ACE_INET_Addr& rhs)
{
  return *this = NetworkAddress(rhs);
}

bool NetworkAddress::operator==(const NetworkAddress& rhs) const
{
  return memcmp(&inet_addr_, &rhs.inet_addr_, sizeof (inet_addr_)) == 0;
}

bool NetworkAddress::operator!=(const NetworkAddress& rhs) const
{
  return memcmp(&inet_addr_, &rhs.inet_addr_, sizeof (inet_addr_)) != 0;
}

bool NetworkAddress::operator<(const NetworkAddress& rhs) const
{
  return memcmp(&inet_addr_, &rhs.inet_addr_, sizeof (inet_addr_)) < 0;
}

bool NetworkAddress::addr_bytes_equal(const NetworkAddress& rhs) const
{
  if (inet_addr_.in4_.sin_family == AF_INET) {
    return memcmp(&inet_addr_.in4_.sin_addr, &rhs.inet_addr_.in4_.sin_addr, sizeof (inet_addr_.in4_.sin_addr)) == 0;
#if defined (ACE_HAS_IPV6)
  } else if (inet_addr_.in6_.sin6_family == AF_INET6) {
    return memcmp(&inet_addr_.in6_.sin6_addr, &rhs.inet_addr_.in6_.sin6_addr, sizeof (inet_addr_.in6_.sin6_addr)) == 0;
#endif
  }
  return 0;
}

ACE_INET_Addr NetworkAddress::to_addr() const
{
  ACE_INET_Addr result;
  result.set_addr(const_cast<ip46*>(&inet_addr_), sizeof (inet_addr_));
  return result;
}

void NetworkAddress::to_addr(ACE_INET_Addr& addr) const
{
  addr.set_addr(const_cast<ip46*>(&inet_addr_), sizeof (inet_addr_));
}

int16_t NetworkAddress::get_type() const
{
  return static_cast<int16_t>(inet_addr_.in4_.sin_family);
}

void NetworkAddress::set_type(int16_t type)
{
  inet_addr_.in4_.sin_family = type;
}

uint16_t NetworkAddress::get_port_number() const
{
  if (inet_addr_.in4_.sin_family == AF_INET) {
    return ACE_NTOHS(inet_addr_.in4_.sin_port);
#if defined (ACE_HAS_IPV6)
  } else if (inet_addr_.in6_.sin6_family == AF_INET6) {
    return ACE_NTOHS(inet_addr_.in6_.sin6_port);
#endif
  }
  return 0;
}

void NetworkAddress::set_port_number(uint16_t port)
{
  if (inet_addr_.in4_.sin_family == AF_INET) {
    inet_addr_.in4_.sin_port = ACE_HTONS(port);
#if defined (ACE_HAS_IPV6)
  } else if (inet_addr_.in6_.sin6_family == AF_INET6) {
    inet_addr_.in6_.sin6_port = ACE_HTONS(port);
#endif
  }
}

bool NetworkAddress::is_any() const
{
  if (inet_addr_.in4_.sin_family == AF_INET) {
    return inet_addr_.in4_.sin_addr.s_addr == INADDR_ANY;
#if defined (ACE_HAS_IPV6)
  } else if (inet_addr_.in6_.sin6_family == AF_INET6) {
    return IN6_IS_ADDR_UNSPECIFIED(&inet_addr_.in6_.sin6_addr);
#endif
  }
  return false;
}

bool NetworkAddress::is_loopback() const
{
  if (inet_addr_.in4_.sin_family == AF_INET) {
    // RFC 3330 defines loopback as any address with 127.x.x.x
    return (ACE_HTONL(inet_addr_.in4_.sin_addr.s_addr) & 0xFF000000) == (INADDR_LOOPBACK & 0XFF000000);
#if defined (ACE_HAS_IPV6)
  } else if (inet_addr_.in6_.sin6_family == AF_INET6) {
    return IN6_IS_ADDR_LOOPBACK(&inet_addr_.in6_.sin6_addr);
#endif
  }
  return false;
}

bool NetworkAddress::is_private() const
{
  if (inet_addr_.in4_.sin_family == AF_INET) {
    // private address classes are10.x.x.x/8
    return ((ACE_HTONL(inet_addr_.in4_.sin_addr.s_addr) & 0xFF000000) == 0x0A000000 ||
            (ACE_HTONL(inet_addr_.in4_.sin_addr.s_addr) & 0xFFF00000) == 0xAC100000 ||
            (ACE_HTONL(inet_addr_.in4_.sin_addr.s_addr) & 0xFFFF0000) == 0xC0A80000);
  }
  return false;
}

bool NetworkAddress::is_uniquelocal() const
{
#if defined (ACE_HAS_IPV6)
  if (inet_addr_.in6_.sin6_family == AF_INET6) {
    return (inet_addr_.in6_.sin6_addr.s6_addr[0] & 0xFE) == 0xFC;
  }
#endif
  return false;
}

bool NetworkAddress::is_linklocal() const
{
#if defined (ACE_HAS_IPV6)
  if (inet_addr_.in6_.sin6_family == AF_INET6) {
    return IN6_IS_ADDR_LINKLOCAL(&inet_addr_.in6_.sin6_addr);
  }
#endif
  return false;
}

bool NetworkAddress::is_sitelocal() const
{
#if defined (ACE_HAS_IPV6)
  if (inet_addr_.in6_.sin6_family == AF_INET6) {
    return IN6_IS_ADDR_SITELOCAL(&inet_addr_.in6_.sin6_addr);
  }
#endif
  return false;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
