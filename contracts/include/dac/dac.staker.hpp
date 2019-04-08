#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>
#include <math.h>   /* floor */

namespace eosiosystem {
class system_contract;
}

namespace eosio {
  
typedef uint64_t uuid;
typedef uint32_t unix_time;

using std::string;

class [[eosio::contract("eosio.staker")]] staker : public contract {
public:
  name VOTER_CONTRACT = name("votertestnet");
  symbol MG_SYMBOL = symbol("MG", 4);
  
  using contract::contract;
  
  void handle_transfer( name from, name to, asset quantity, std::string memo );

private:
  void handle_staking( asset quantity );
  
  // the bable from voting contract: used for staking
  struct dapps_table {
    uuid  id;
    name      dappname;
    std::string    description;
    asset     votes;
    
    uint64_t primary_key()const { return id; }
  };
  typedef eosio::multi_index< "dapps"_n, dapps_table > dapps;
  
  // the table from voting contract: used to get totalvotes
  struct Config {
    uuid last_id;
    asset totalvotes;
    // time when the last drop was held
    unix_time last_drop_time; // todo: 3*24*3600
    // time of the next drop
    unix_time next_drop_time; // todo: 3*24*3600 + 1980 year
    uint64_t primary_key()const { return name("settings").value; }
  };
  typedef eosio::multi_index< "settings"_n, Config > settings;
  
  // typedef singleton<name("settings"), Config> SingletonType;
  // SingletonType ConfigSingleton;
  
  // gets the config from the voter contract
  asset _get_voter_total_votes();
};

} /// namespace eosio