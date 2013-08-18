SQLite Cxx Project

A C++ client library for SQLite

Example
sqlite_connection* conn = new sqlite_connection;
if(conn->open(L"some_path in unicode")){
  std::shared_ptr<sqlite_command> cmd = conn->create_command(L"Create table example(id INTEGER PRIMARY KEY, value TEXT)");
  if(cmd->execute()){
    //we done/
  }
}else {//handle errors
}

//Do query
std::shared_ptr<sqlite_command> cmd = conn->create_command(L"select id,value from example where id=?");
cmd->bind_int64(1, 100);
std::shared_ptr<sqlite_reader> reader = cmd->execute_reader();
while(reader->read()){
  std::wstring value = reader->wstr(1);
  //do something with value
}//end while

================================================================================

//query with qryhelper
std::list<std::shared_ptr<example>> query(std::shared_ptr<QueryExpression> exp){
  wstring sql(L"select id, value from example");
  if(exp){
    sql.append(L" WHERE ");
    sql.append(exp->GetExpression());
  }
  sql .append(L" ORDER BY id ASC");
  shared_ptr<sqlite_command> cmd = get_connection()->create_command(sql.c_str());
  if(exp){
    int index = 1;
    exp->Bind(*cmd, index);
  }
  std::shared_ptr<sqlite_reader> reader = cmd->execute_reader();
  std::list<std::shared_ptr<example>> result;
  while(reader->read()){
    //build the list with reader
  }
  return result;
}

//call query as below
//get objects that with id greater than 100 but less than or equals to 150
auto results = query(Property("id") > 100 && Property("id") <= 150);
