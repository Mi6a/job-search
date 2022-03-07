#pragma once

#include <memory>

#include "cpp20/unordered_set"
#include "cpp20/unordered_map"

#include "OrderCache.h"

// Implementation selection explanation:
//   1. The decisions were made toward maximizing the speed at the memory expense
//   2. 
//   2. Orders are kept in the unordered_set, and hashed/searched by m_orderId
//   3. In addition we 

//
class OrderCacheImpl : public OrderCacheInterface {
public:
   // add order to the cache
   virtual void addOrder(Order order);

   // remove order with this unique order id from the cache
   virtual void cancelOrder(const std::string& orderId);

   // remove all orders in the cache for this user
   virtual void cancelOrdersForUser(const std::string& user);

   // remove all orders in the cache for this security with qty >= minQty
   virtual void cancelOrdersForSecIdWithMinimumQty(const std::string& securityId, unsigned int minQty);

   // return the total qty that can match for the security id
   virtual unsigned int getMatchingSizeForSecurity(const std::string& securityId);

   // return all orders in cache in a vector
   virtual std::vector<Order> getAllOrders() const;

private:
/*   using StrShared = std::shared_ptr<std::string>;

   struct StrSharedEqual {
      using is_transparent = void; // for find() override

      bool operator()(const StrShared& ss1, const StrShared& ss2) const  noexcept {
         return *ss1 == *ss2;
      }
      bool operator()(const std::string& s1, const StrShared& ss2) const  noexcept {
         return s1 == *ss2;
      }
      bool operator()(const StrShared& ss1, const std::string& s2) const  noexcept {
         return *ss1 == s2;
      }
   };

   //
   struct StrSharedHash {
      using is_transparent = void; // for find() override

      std::size_t operator()(const StrShared& ss) const noexcept {
         return std::hash<std::string>{}(*ss);
      }

      std::size_t operator()(const std::string& s) const noexcept {
         return std::hash<std::string>{}(s);
      }
   };

   using PtrStrSet = std::unordered_set<StrShared, StrSharedHash, StrSharedEqual>; 
*/
   using PtrOrder = std::shared_ptr<Order>;

   struct PtrOrderEqual {
      using is_transparent = void; // for find() override

      bool operator()(const PtrOrder& po1, const PtrOrder& po2) const  noexcept {
         return po1->orderId() == po2->orderId();
      }
      bool operator()(const std::string& idOrder1, const PtrOrder& po2) const  noexcept {
         return idOrder1 == po2->orderId();
      }
      bool operator()(const PtrOrder& po1, const std::string& idOrder2) const  noexcept {
         return po1->orderId() == idOrder2;
      }
   };

   //
   struct PtrOrderHash {
      using is_transparent = void; // for find() override

      std::size_t operator()(const PtrOrder& po) const noexcept {
         return std::hash<std::string>{}(po->orderId());
      }

      std::size_t operator()(const std::string& s) const noexcept {
         return std::hash<std::string>{}(s);
      }
   };



   using PtrOrderSet = std::unordered_set<PtrOrder, PtrSharedHash, PtrSharedEqual>;

};