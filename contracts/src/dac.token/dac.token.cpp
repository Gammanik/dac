#include "./dac.token.hpp"

namespace eosio {

void token::create( name   issuer,
                    asset  maximum_supply )
{
  require_auth( _self );
  
  auto sym = maximum_supply.symbol;
  eosio_assert( sym.is_valid(), "invalid symbol name" );
  eosio_assert( maximum_supply.is_valid(), "invalid supply");
  eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");
  
  stats statstable( _self, sym.code().raw() );
  auto existing = statstable.find( sym.code().raw() );
  eosio_assert( existing == statstable.end(), "token with symbol already exists" );
  
  statstable.emplace( _self, [&]( auto& s ) {
    s.supply.symbol = maximum_supply.symbol;
    s.max_supply    = maximum_supply;
    s.issuer        = issuer;
  });
}


void token::issue( name to, asset quantity, string memo )
{
  auto sym = quantity.symbol;
  eosio_assert( sym.is_valid(), "invalid symbol name" );
  eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );
  
  stats statstable( _self, sym.code().raw() );
  auto existing = statstable.find( sym.code().raw() );
  eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
  const auto& st = *existing;
  
  require_auth( st.issuer );
  eosio_assert( quantity.is_valid(), "invalid quantity" );
  eosio_assert( quantity.amount > 0, "must issue positive quantity" );
  
  eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
  eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");
  
  statstable.modify( st, same_payer, [&]( auto& s ) {
    s.supply += quantity;
  });
  
  // todo: here I'm always paying for the RAM
  add_balance( st.issuer, quantity, st.issuer );
  
  if( to != st.issuer ) {
    SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} },
                        { st.issuer, to, quantity, memo }
    );
  }
}

void token::airgrab( name grabber ) {
  
  // add _self in case of giveaway
  eosio_assert( has_auth(grabber) || has_auth(_self), ("you are not who you say you are: " + grabber.to_string()).c_str());
  eosio_assert( is_account(grabber), ("grabber account does not exist: " + grabber.to_string()).c_str() );
  
  // check if an account already has MG tokens
  accounts acnts( _self, grabber.value );
  symbol mg_symbol = symbol("MG", 4);
  auto it = acnts.find( mg_symbol.code().raw() );
  eosio_assert( it == acnts.end(), "you have already grabbed the MG token from this account." );
  
  
  // get the EOS balance of the account
  accounts acnts_eosio( name("eosio.token"), grabber.value );
  symbol eos_symbol = symbol("EOS", 4);
  const auto& grabber_balance = acnts_eosio.get( eos_symbol.code().raw(), "no balance object found" );
  eosio_assert( grabber_balance.balance.amount > 0, "you have no unstaked EOS to get your airgrab" );
  asset mg_tokens = asset(grabber_balance.balance.amount, mg_symbol);
  
  // maximum tokens per account. as a precision = 4 then multiply by 1000
  int MAX_LIMIT = 500 * 10000; // 500 MG is a limit
  mg_tokens.amount = ( mg_tokens.amount < MAX_LIMIT ) ?
                     mg_tokens.amount :
                     MAX_LIMIT;
  
  // todo:: in case of giveaway make _self pay for the RAM!!!!
  
  // issue new tokens for the grabber
  stats statstable( _self, mg_symbol.code().raw() );
  auto existing = statstable.find( mg_symbol.code().raw() );
  eosio_assert( existing != statstable.end(), "token with MG symbol does not exist yet" );
  const auto& st = *existing;
  
  eosio_assert( mg_tokens.symbol == st.supply.symbol, "symbol precision mismatch" );
  eosio_assert( mg_tokens.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");
  
  statstable.modify( st, same_payer, [&]( auto& s ) {
    s.supply += mg_tokens;
  });
  
  add_balance( _self, mg_tokens, grabber );
  
  // requires @eosio@code repmission for _self
  if( grabber != st.issuer ) {
    SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} },
                        { _self, grabber, mg_tokens, "" }
    );
  }
}

void token::drop( name grabber ) {
  eosio_assert( has_auth(_self), ("only an account owner could make drops. grabber: " + grabber.to_string()).c_str());
  
  if( !is_account(grabber) ) {
    // todo: erase the account from the table?
    return;
  }
  
  // check if an account already has MG tokens
  accounts acnts( _self, grabber.value );
  symbol mg_symbol = symbol("MG", 4);
  auto it = acnts.find( mg_symbol.code().raw() );
  // eosio_assert( it == acnts.end(), "you have already grabbed the MG token from this account." );
  
  // get the EOS balance of the account
  accounts acnts_eosio( name("eosio.token"), grabber.value );
  symbol eos_symbol = symbol("EOS", 4);
  const auto& grabber_balance = acnts_eosio.find( eos_symbol.code().raw() );
  // acnts_eosio.get( eos_symbol.code().raw(), "no balance object found" );
  
  if ( grabber_balance == acnts_eosio.end() ) return;
  
  // "you have no unstaked EOS to get your airgrab"
  if ( !(grabber_balance->balance.amount > 0) ) return;
  
  asset mg_tokens = asset(grabber_balance->balance.amount, mg_symbol);
  // maximum tokens per account. as a precision = 4 then multiply by 1000
  int MAX_LIMIT = 500 * 10000; // 500 MG is a limit
  mg_tokens.amount = ( mg_tokens.amount < MAX_LIMIT ) ?
                     mg_tokens.amount :
                     MAX_LIMIT;
  
  // issue new tokens for the grabber
  stats statstable( _self, mg_symbol.code().raw() );
  auto existing = statstable.find( mg_symbol.code().raw() );
  eosio_assert( existing != statstable.end(), "token with MG symbol does not exist yet" );
  const auto& st = *existing;
  
  eosio_assert( mg_tokens.symbol == st.supply.symbol, "symbol precision mismatch" );
  eosio_assert( mg_tokens.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");
  
  statstable.modify( st, same_payer, [&]( auto& s ) {
    s.supply += mg_tokens;
  });
  //in case of giveaway _self pays for the RAM
  add_balance( _self, mg_tokens, _self );
  
  if( grabber != st.issuer ) { // requires @eosio@code repmission for _self
    SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} },
                        { _self, grabber, mg_tokens, "" }
    );
  }
  
  // todo: https://github.com/EOSIO/eos/issues/5025 =-- return an iterator?
  // grabnames grabstable( _self, _self.value );
  // auto itr_grabnames = grabstable.find( grabber.value );
  // eosio_assert( itr_grabnames != grabstable.end(), ("grabber is not in the grabstable: " + grabber.to_string()).c_str() );
  // grabstable.erase(itr_grabnames);
}

void token::addnames( std::vector<name>& names ) {
  require_auth(_self);
  
  grabnames grabstable( _self, _self.value );
  eosio_assert(names.size() > 0, "the array of names you're trying to add is empty");
  
  for ( const auto& iname: names ) {
    auto itr_grabnames = grabstable.find( iname.value );
    
    // todo: just make if/else without assert?
    eosio_assert( itr_grabnames == grabstable.end(), ("this account is already in the table: " + iname.to_string()).c_str());
    
    grabstable.emplace( _self, [&]( auto& a ){
      a.acc = iname;
    });
  }
}


// pLimit is the (max) number of elements
// which shall be deleted in each index
// before the next transaction is created
// todo: add arguments: uint64_t pLimit, uint128_t pLastId

void token::giveaway() {
  
  grabnames table( _self, _self.value );
  
  // todo: partial giveaway
  for (auto itr = table.begin(); itr != table.end(); ++itr) {
    drop(itr->acc);
  }
}

void token::retire( asset quantity, string memo )
{
  auto sym = quantity.symbol;
  eosio_assert( sym.is_valid(), "invalid symbol name" );
  eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );
  
  stats statstable( _self, sym.code().raw() );
  auto existing = statstable.find( sym.code().raw() );
  eosio_assert( existing != statstable.end(), "token with symbol does not exist" );
  const auto& st = *existing;
  
  require_auth( st.issuer );
  eosio_assert( quantity.is_valid(), "invalid quantity" );
  eosio_assert( quantity.amount > 0, "must retire positive quantity" );
  
  eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
  
  statstable.modify( st, same_payer, [&]( auto& s ) {
    s.supply -= quantity;
  });
  
  sub_balance( st.issuer, quantity );
}

void token::burn( name account, asset quantity, string memo ) {
  auto sym = quantity.symbol;
  eosio_assert( sym.is_valid(), "invalid symbol name" );
  eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );
  
  stats statstable( _self, sym.code().raw() );
  auto existing = statstable.find( sym.code().raw() );
  eosio_assert( existing != statstable.end(), "token with symbol does not exist" );
  const auto& st = *existing;
  
  require_auth( account );
  eosio_assert( quantity.is_valid(), "invalid quantity" );
  eosio_assert( quantity.amount > 0, "must retire positive quantity" );
  
  eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
  
  statstable.modify( st, same_payer, [&]( auto& s ) {
    s.supply -= quantity;
  });
  
  
  if( account != st.issuer ) {
    SEND_INLINE_ACTION( *this, transfer, { {account, "active"_n} },
                        { account, st.issuer, quantity, memo }
    );
  }
  
  // todo: check if less than 50% left
  
  accounts from_acnts( _self, _self.value );
  const auto& from = from_acnts.get( quantity.symbol.code().raw(), "no balance object found" );
  eosio_assert( from.balance.amount >= quantity.amount, "now it's impossible to burn more tokens tnan the contract has at once" );
  // todo: should eosio_assert(  )
  sub_balance( st.issuer, quantity );
  
}

void token::transfer( name    from,
                      name    to,
                      asset   quantity,
                      string  memo )
{
  eosio_assert( from != to, "cannot transfer to self" );
  require_auth( from );
  eosio_assert( is_account( to ), "to account does not exist");
  auto sym = quantity.symbol.code();
  stats statstable( _self, sym.raw() );
  const auto& st = statstable.get( sym.raw() );
  
  require_recipient( from );
  require_recipient( to );
  
  eosio_assert( quantity.is_valid(), "invalid quantity" );
  eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
  eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
  eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );
  
  auto payer = has_auth( to ) ? to : from;
  
  sub_balance( from, quantity );
  add_balance( to, quantity, payer );
}

void token::sub_balance( name owner, asset value ) {
  accounts from_acnts( _self, owner.value );
  
  const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
  eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );
  
  from_acnts.modify( from, owner, [&]( auto& a ) {
    a.balance -= value;
  });
}

void token::add_balance( name owner, asset value, name ram_payer )
{
  accounts to_acnts( _self, owner.value );
  auto to = to_acnts.find( value.symbol.code().raw() );
  if( to == to_acnts.end() ) {
    to_acnts.emplace( ram_payer, [&]( auto& a ){
      a.balance = value;
    });
  } else {
    to_acnts.modify( to, same_payer, [&]( auto& a ) {
      a.balance += value;
    });
  }
}

void token::open( name owner, const symbol& symbol, name ram_payer )
{
  require_auth( ram_payer );
  
  auto sym_code_raw = symbol.code().raw();
  
  stats statstable( _self, sym_code_raw );
  const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );
  eosio_assert( st.supply.symbol == symbol, "symbol precision mismatch" );
  
  accounts acnts( _self, owner.value );
  auto it = acnts.find( sym_code_raw );
  if( it == acnts.end() ) {
    acnts.emplace( ram_payer, [&]( auto& a ){
      a.balance = asset{0, symbol};
    });
  }
}

void token::dropnames( ) {
  require_auth( _self );
  
  grabnames _grabnames( _self, _self.value );
  for(auto itr = _grabnames.begin(); itr != _grabnames.end();) {
    itr = _grabnames.erase(itr);
  }
}


void token::close( name owner, const symbol& symbol )
{
  require_auth( owner );
  accounts acnts( _self, owner.value );
  auto it = acnts.find( symbol.code().raw() );
  eosio_assert( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
  // todo: now for testing puproses uncomment the line below
  // eosio_assert( it->balance.amount == 0, "Cannot close because the balance is not zero." );
  acnts.erase( it );
}

} /// namespace eosio



// EOSIO_ABI( eosio::token, (create)(issue)(airgrab)(transfer) )
EOSIO_DISPATCH( eosio::token, (create)(issue)(airgrab)(burn)(addnames)(giveaway)(transfer)(open)(retire)(dropnames) )