#include <string>
#include <vector>
#include <eosiolib/eosio.hpp>
// #include <eosiolib/time.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/action.h>
#include "../rankblock.token/rankblock.token.hpp"

using namespace eosio;
using namespace std;

#define TRANSFER_MEMO_FOR_ADD_ITEM "additem"

class rankblock : public contract
{
  static constexpr uint64_t code = N(rankblockctr);
  static constexpr uint64_t token = N(rankblocktkn);
  const symbol_type rankblock_SYMBOL = eosio::symbol_type(eosio::string_to_symbol(4, "RB"));
  typedef uint64_t uuid;
  typedef uint64_t score;
public:
  enum enum_rank_item_state {
    E_RANK_ITEM_CREATED,
    E_RANK_ITEM_EDITED,
    E_RANK_ITEM_DEAD,
    E_RANK_ITEM_LEGEND
  };

  enum enum_rank_item_type {
    E_RANK_ITEM_SHOE,
    E_RANK_ITEM_WATCH,
    E_RANK_ITEM_CAR
  };

  rankblock(account_name self)
    :contract(self),
    _state_config(_self, _self)
    {
    }

  //@abi action
  void setconfig( uint32_t reward_amount_vote, // 5000
                  uint32_t reward_amount_stamp, // 5000
                  uint32_t max_item_cnt, // 20
                  uint32_t item_life_time, // 14 DAYS
                  uint32_t target_score_to_be_legend, // 10000
                  uint32_t cost_amount_per_add_item, // 100000
                  uint8_t score_amout_per_vote) // 10
  {
    require_auth(_self);

    _state_config.set(config{
      reward_amount_vote,
      reward_amount_stamp,
      max_item_cnt,
      item_life_time, // SEC
      target_score_to_be_legend,
      cost_amount_per_add_item,
      score_amout_per_vote
    },
    _self);
  };
  
  // // @abi action
  // void adduser(account_name user)
  // {
  //   require_auth(user);
  //   users usrs(_self, user);
  //   auto itr_usr = usrs.find(user);
  //   eosio_assert(itr_usr == usrs.end(), "error: This account is already exist.");
  //   usrs.emplace(_self, [&](auto &a) {
  //   });
  // }

   //@abi action
  void createitem(account_name user)
  {
    require_auth(user);
    rank_items ritms(_self, _self);
    ritms.emplace(_self, [&](auto &a) {
      a.rank_item_id = ritms.available_primary_key();
      a.rank_item_state = E_RANK_ITEM_CREATED;
    });
  }

  //@abi action
  void setitemdata(account_name user, uuid rank_item_id, uuid rank_item_data_id, uint8_t rank_item_type)
  {
    require_auth(user);
    rank_items ritms(_self, _self);
    auto& item = ritms.get(rank_item_id, "e.1"); // There is no item for this input rank item id.
    eosio_assert(item.rank_item_state == E_RANK_ITEM_CREATED, "e.2"); // It is invalid state for set item.
    ritms.modify(item, 0, [&](auto &a) {
      a.rank_item_data_id = rank_item_data_id;
      a.rank_item_last_voted = now(); // Editing is same as first voting.
      a.rank_item_state = E_RANK_ITEM_EDITED;
      a.rank_item_type = rank_item_type;
    });
  }

  //@abi action
  void voteitem(account_name user, uuid rank_item_id, uint64_t vote_amount) {
    require_auth(user);

    rank_items ritms(_self, _self);
    auto& item = ritms.get(rank_item_id, "e.1"); // There is no item for this input rank item id.
    eosio_assert(item.rank_item_state == E_RANK_ITEM_EDITED, "e.2"); // It is invalid state for vote item.
    ritms.modify(item, 0, [&](auto &a) {
      a.rank_item_score += _state_config.get().score_amout_per_vote;
      a.rank_item_last_voted = now();
    });

    // issue reward
    asset reward = asset(_state_config.get().reward_amount_vote, rankblock_SYMBOL);
    vector<permission_level> perlvs;
    permission_level per_token = permission_level{ token, N(active) };
    permission_level per = permission_level{ code, N(active) };
    perlvs.push_back(per_token);
    perlvs.push_back(per);
    action(perlvs, token, N(issue), std::make_tuple(user, reward, std::string(""))).send();
  };

  // //@abi action
  // void commentitem(account_name user, string comment) {

  // }

  //@abi action
  void transfer (uint64_t receiver, uint64_t code) {
    auto data = unpack_action_data<rankblocktoken::transfer_args>();
    if(data.from == _self || data.to != _self) {
      return;
    }
    
    eosio_assert(data.quantity.symbol == rankblock_SYMBOL, "error: Only enable RB token.");
    eosio_assert(data.quantity.is_valid(), "error: Invalid token transfer");
    eosio_assert(data.quantity.amount > 0, "error: Quantity must be positive");

    if (data.memo.empty())
      return;

    // This is for blocking invalid transfer with memo.
    // In normal case, it is also transferred by own token contract. So it has to be same.
    if (data.memo == TRANSFER_MEMO_FOR_ADD_ITEM) {
      eosio_assert(data.quantity.amount == _state_config.get().cost_amount_per_add_item, "e."); //Invalid Amount is transferred.
      createitem(receiver);
    }
  }

private:
  // uint64_t gettokenamount(account_name owner) {
  //   rankblocktoken::accounts acc(N(rankblocktoken), owner);
  //   auto itr_acc = acc.find(rankblock_SYMBOL.name(), "error: There is no account for input account name.");

  //   if (itr_acc != acc.end()) {
  //     return itr_acc->balance.amount;
  //   } else {
  //     return 0;
  //   }
  // }

  // // This function is used to swap endianess for uint64_t's for key and javascript 58-bit int limit issues.
  // uint64_t eparticlectr::swapEndian64(uint64_t X) {
  //   uint64_t x = (uint64_t) X;
  //   x = (x & 0x00000000FFFFFFFF) << 32 | (x & 0xFFFFFFFF00000000) >> 32;
  //   x = (x & 0x0000FFFF0000FFFF) << 16 | (x & 0xFFFF0000FFFF0000) >> 16;
  //   x = (x & 0x00FF00FF00FF00FF) << 8  | (x & 0xFF00FF00FF00FF00) >> 8;
  //   return x;
  // }

  //@abi table config i64
  struct config
  {
    uint32_t reward_amount_vote;
    uint32_t reward_amount_stamp;
    uint32_t max_item_cnt;
    uint32_t item_life_time;
    uint32_t target_score_to_be_legend;
    uint32_t cost_amount_per_add_item;
    uint8_t score_amout_per_vote;

    EOSLIB_SERIALIZE(config, (reward_amount_vote)(reward_amount_stamp)(max_item_cnt)(item_life_time)(target_score_to_be_legend)(cost_amount_per_add_item)(score_amout_per_vote))
  };

  typedef singleton<N(config), config> state_config;
  state_config _state_config;

  // //@abi table user i64
  // struct user
  // {
  //   score user_score;
  //   EOSLIB_SERIALIZE(user, (user_score))
  // };

  // using users = eosio::multi_index<N(user), user>;

  //@abi table rankitem i64
  struct rankitem
  {
    uuid rank_item_id;
    account_name rank_item_creator;
    uuid rank_item_data_id;
    score rank_item_score;
    uint32_t rank_item_last_voted;
    uint8_t rank_item_type; // max 256
    uint8_t rank_item_state;

    uuid primary_key() const { return rank_item_id; }
    account_name get_creator() const { return rank_item_creator; }
    uuid get_score() const { return rank_item_score; }
    uint64_t get_state() const { return rank_item_state; }
    uint64_t get_type() const { return rank_item_type; }

    EOSLIB_SERIALIZE(rankitem, (rank_item_id)(rank_item_creator)(rank_item_data_id)(rank_item_score)(rank_item_last_voted)(rank_item_type)(rank_item_state))
  };

  using rank_items = eosio::multi_index<N(rankitem), rankitem,
                                    indexed_by<N(by_creator), const_mem_fun<rankitem, account_name, &rankitem::get_creator>>,
                                    indexed_by<N(by_score), const_mem_fun<rankitem, score, &rankitem::get_score>>,
                                    indexed_by<N(by_state), const_mem_fun<rankitem, uint64_t, &rankitem::get_state>>,
                                    indexed_by<N(by_type), const_mem_fun<rankitem, uint64_t, &rankitem::get_type>>>;
};


// #define EOSIO_ABI_EX( TYPE, MEMBERS ) \
// extern "C" { \
//    void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
//       if( action == N(onerror)) { \
//          /* onerror is only valid if it is for the "eosio" code account and authorized by "eosio"'s "active permission */ \
//          eosio_assert(code == N(eosio), "onerror action's are only valid from the \"eosio\" system account"); \
//       } \
//       auto self = receiver; \
//       if( code == self || code == N(rankblocktkn) || action == N(onerror) ) { \
//          TYPE thiscontract( self ); \
//          switch( action ) { \
//             EOSIO_API( TYPE, MEMBERS ) \
//          } \
//          /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
//       } \
//    } \
// }
// EOSIO_ABI_EX(rankblock, (setconfig)(adduser)(createitem)(setitemdata)(voteitem)(transfer))
EOSIO_ABI(rankblock, (setconfig)(createitem)(setitemdata)(voteitem)(transfer))