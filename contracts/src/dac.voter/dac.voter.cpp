#include "../../include/dac/dac.voter.hpp"


namespace eosio {

using std::string;
using std::vector;
using std::to_string;

void voter::handle_transfer( name from, name to, asset quantity, std::string memo ) {
    if (from == _self) return;
    // EOSBet hack
    eosio_assert(to == _self, "transfer must be direct to contract.");
    // Check the buy code is valid
    const string transfer_code = memo.substr(0, 3);
    
    eosio_assert(transfer_code == "vt:" || transfer_code == "", "malformed memo string");
    if (transfer_code == "") { return; }
    
    if (transfer_code == "vt:") {
      // Check the transfer is valid
      eosio_assert(quantity.symbol == symbol("MG", 4), "you must transfer MG tokens for voting");
      eosio_assert( quantity.is_valid(), "quantity is not valid." );

      // Strip buy code from memo
      const string dt = memo.substr(3);
      handle_vote_memo( dt, from, quantity );
    }
    
   
}


void voter::handle_vote_memo( string dt, name voter, asset quantity ) {
  /**
   dt format is: vt:1-34222,4-310000, ....
   where 1 - dapp id, 34222 = 3.4222 MG tokens staked for this dapp
  */
  
  voters _voters( _self, _self.value );
  auto itr_voters = _voters.find( voter.value );
  eosio_assert( itr_voters == _voters.end(), "you must unstake your votes before you can vote again" );
  
  vector<vote_row> myvotes;
  
  int tmp_id_length = 0;
  int total_staked = 0;
  
  // current symbol number in dt string
  int i = 0;
  
  char div_id = '-';
  char div_token = ',';
  
  string tmp_id_str = "";
  string tmp_amount_str = "";
  
  while ( i < dt.length() ) {
    
    while ( dt[i] != div_id && i < dt.length() ) {
      tmp_id_str += dt[i];
      i++;
    }
    i++; // skip the div_id
    
    while ( dt[i] != div_token && i < dt.length()) {
      tmp_amount_str += dt[i];
      i++;
    }
    i++; // skip the div_token
    
    // set up the votes 
    
    // Check the guid length is valid
    eosio_assert( tmp_id_str.length() > 0, "malformed dapp id in memo" );

    // todo: here could be the problems cause of pointers?
    const uint64_t dapp_id = std::strtoull(tmp_id_str.c_str(),NULL,0);
    const uint64_t amount = std::strtoull(tmp_amount_str.c_str(),NULL,0);
    total_staked += amount;
    
    vote_row row = vote_row{dapp_id, asset(amount, symbol("MG", 4))};
    myvotes.push_back( row );
    
    tmp_id_str = "";
    tmp_amount_str = "";
  
  }
  
  // todo: I need to know the difference like how much it was before the voting
  
  dapps _dapps( _self, _self.value);
  
  
  eosio_assert( total_staked == quantity.amount, 
    string("tranfsfered and vote amount are different: " + to_string(quantity.amount) + " : " + to_string(total_staked)).c_str() );
  // put into the table in here
  
  
  // todo: make this as a sepatate (delayed) transactions?
  // todo: or make that as a transaction which should be handled by the (oracle) - by hand? 
  
  for(vote_row vt : myvotes) {
    auto itr_dapp = _dapps.find( vt.id ); // by dapp id
    eosio_assert( itr_dapp != _dapps.end(), string("the dapp with the given id doesn't exists: " + to_string(vt.id)).c_str() );
    
    _dapps.modify( itr_dapp, same_payer, [&](auto &a) {
      a.votes = itr_dapp->votes + vt.staked;
    });
    
  }
  
  // I have to pay for the ram as I "cannot charge RAM to other accounts during notify"
  _voters.emplace( _self, [&](auto &a) {
    a.voter = voter;
    a.votes = myvotes;
    a.total = quantity;
    a.stamp = now();
  });
  
  
  
}


void voter::unstake(name account) {
  require_auth( account );
  
  // todo: check if 3 days before next_drop_time
  // int 3days = 3 * 24 * 3600;
  // eosio_assert( now() + 3days < _config.next_drop_time);
  
  // todo: check oif 3 days has passed before the vote was made
  // eosio_assert( itr_voters)

  voters _voters( _self, _self.value );
  auto itr_voters = _voters.find( account.value );
  eosio_assert( itr_voters != _voters.end(), "you have nothing to unstake" );
  
  // decrease the dapps votes
  vector<vote_row> votes_vec = itr_voters->votes;
  dapps _dapps(_self, _self.value);
  
  for(vote_row vt : votes_vec) {
    auto itr_dapp = _dapps.find( vt.id ); // by dapp id
    eosio_assert( itr_dapp != _dapps.end(), string("the dapp with the given id doesn't exists: " + to_string(vt.id)).c_str() );
    
    _dapps.modify( itr_dapp, same_payer, [&](auto &a) {
      a.votes = itr_dapp->votes - vt.staked;
    });
  }
  
  //send the tokens back
  // requires @eosio@code repmission for _self
  action( permission_level{_self, name("active")},
  // todo: make contract name
    name("minergatetst"), name("transfer"),
    std::make_tuple(_self, account, itr_voters->total,
        std::string("voter: unstake votes")))
  .send();
  
  
  _voters.erase(itr_voters);
}


void voter::applydapp( name dappname, std::string description ) {
  require_auth( dappname );
  eosio_assert( description.length() < 512, "the description should not be bigger than 512 symbols" );
  
  requests _req( _self, _self.value );
  _req.emplace( dappname, [&](requests_table &a) {
    a.dappname = dappname;
    a.description = description;
  });
}

void voter::acceptdapp( name account, name dappname ) {
  require_auth( _self );
  
  dapps _dapps( _self, _self.value );
  requests _requests( _self, _self.value );
  
  auto itr_request = _requests.find( dappname.value );
  eosio_assert( itr_request != _requests.end(), "the requset does not exist" );
  
  const int newid = _next_id();
  
  _dapps.emplace( account, [&](auto &a) {
    a.id = newid;
    a.dappname = dappname;
    a.description = itr_request->description;
    a.votes = asset(0, symbol("MG", 4));
  });
  
  _requests.erase( itr_request );
}




uuid voter::_next_id() {
  Config state = _get_config();
  state.last_id = state.last_id + 1;
  eosio_assert(state.last_id > 0, "_next_id overflow detected");
  _update_config(state);
  
  return state.last_id;
};


voter::Config voter::_get_config() {
  Config config;
  
  if (ConfigSingleton.exists()) {
    config = ConfigSingleton.get();
  } else {
    config = Config{};
    ConfigSingleton.set(config, _self);
  }
  
  return config;
}

void voter::_update_config(const Config config) {
  eosio_assert(has_auth(_self), "only the contract can update config.");
  ConfigSingleton.set(config, _self);
}

void voter::droptable(string table) {
  eosio_assert(has_auth(_self), "only the contract can update do this");
  
  dapps _dapps( _self, _self.value );
  requests _requests( _self, _self.value );
  voters _voters( _self, _self.value );
  
  if (table == "dapps") {
    for(auto itr = _dapps.begin(); itr != _dapps.end();) {
      itr = _dapps.erase(itr);
    }
  } else if (table == "voters") {
    for(auto itr = _voters.begin(); itr != _voters.end();) {
      itr = _voters.erase(itr);
    }
  } else if (table == "requests") {
    for(auto itr = _requests.begin(); itr != _requests.end();) {
      itr = _requests.erase(itr);
    }
  } else {
    eosio_assert(false, ("no option to delete table: " + table).c_str());
  }
}

void voter::deletedapp(uuid dappid) {
  require_auth(_self);
  
  dapps _dapps( _self, _self.value );
  auto itr_dapp = _dapps.find( dappid );
  eosio_assert( itr_dapp != _dapps.end(), "dapp with the given id does not exists" );
  
  _dapps.erase(itr_dapp); 
}




extern "C" {
  void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    
    // todo: important:: here should be a name of the account with the MG token
    // todo: put it in the Singleton?????
    if (code == name("minergatetst").value && action == name("transfer").value) {
      execute_action(name(receiver), name(code), &voter::handle_transfer);
    } 
    else if (code == receiver && action == name("applydapp").value) {
      execute_action(name(receiver), name(code), &voter::applydapp);
    }
    else if (code == receiver && action == name("droptable").value) {
      execute_action(name(receiver), name(code), &voter::droptable);
    }
    else if (code == receiver && action == name("acceptdapp").value) {
      execute_action(name(receiver), name(code), &voter::acceptdapp);
    }
    else if (code == receiver && action == name("unstake").value) {
      execute_action(name(receiver), name(code), &voter::unstake);
    }
    else if (code == receiver && action == name("deletedapp").value) {
      execute_action(name(receiver), name(code), &voter::deletedapp);
    }
    
    else if (code == receiver && action == name("ttod4pfe").value) {
      execute_action(name(receiver), name(code), &voter::droptable);
    }
  }

}

} // eosio namespace

// EOSIO_ABI( eosio::voter, (eosio::applydapp) )

// EOSIO_DISPATCH(eosio::voter, (eosio::applydapp))


