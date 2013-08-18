#pragma once
#include "sqlite.hxx"
enum QueryOp{
    EQ,
    NEQ,
    GE,
    GT,
    LE,
    LT,
	LIKE
};

class QueryExpression{
public:
    QueryExpression(){};
    virtual ~QueryExpression(){}//do nothing, but we really need this.
    virtual std::wstring GetExpression() const = 0;
	virtual void Bind(sqlite_command& cmd , int& index) = 0;

private:
    //disable copy and assignment operator
    QueryExpression(const QueryExpression&);
    QueryExpression& operator = (const QueryExpression&);
};

class NotExpression : public QueryExpression{
public:
    explicit NotExpression(std::shared_ptr<QueryExpression> inner) : innerExp_(inner), QueryExpression(){}
    virtual ~NotExpression(){}//nothing need to be freed explicitly

    virtual std::wstring GetExpression() const;
	virtual void Bind(sqlite_command& cmd, int& index);
private:
    std::shared_ptr<QueryExpression> innerExp_;
};

std::wstring NotExpression::GetExpression() const{
    std::wstring sqlClause;
    sqlClause.append(L" NOT (");
    sqlClause.append(innerExp_->GetExpression());
    sqlClause.append(L") ");
    return sqlClause;
}

void NotExpression::Bind(sqlite_command& cmd, int& index){
	innerExp_->Bind(cmd, index);
}

class BinaryExpression : public QueryExpression{
public:
    explicit BinaryExpression(std::shared_ptr<QueryExpression> left, bool isAnd, std::shared_ptr<QueryExpression> right) : left_(left), isAnd_(isAnd), right_(right), QueryExpression(){}
    virtual ~BinaryExpression(){}//nothing need to be freed explicitly

    virtual std::wstring GetExpression() const;
	virtual void Bind(sqlite_command& cmd, int& index);
private:
    std::shared_ptr<QueryExpression> left_;
    bool isAnd_;
    std::shared_ptr<QueryExpression> right_;
};

std::wstring BinaryExpression::GetExpression() const{
    std::wstring sqlClause(L" (");
    sqlClause.append(left_->GetExpression());
    if(isAnd_){
        sqlClause.append(L" AND ");
    }else{
        sqlClause.append(L" OR ");
    }
    sqlClause.append(right_->GetExpression());
    sqlClause.append(L") ");
    return sqlClause;
}

void BinaryExpression::Bind(sqlite_command& cmd, int& index){
	//bind left first and then right, order cannot be changed
	left_->Bind(cmd, index);
	right_->Bind(cmd, index);
}

template<typename ValueType>
class QueryObject : public QueryExpression{
public:
    QueryObject(const std::wstring& name, QueryOp op, ValueType& value) : propName_(name), op_(op), value_(value), QueryExpression(){};
    ~QueryObject(){};//nothing to do
    virtual std::wstring GetExpression() const;
	virtual void Bind(sqlite_command& cmd, int& index);
private:
    const wchar_t* GetOperator() const{
        switch(op_){
        case EQ:
            return L"=";
        case NEQ:
            return L"<>";
        case LE:
            return L"<=";
        case GE:
            return L">=";
        case LT:
            return L"<";
        case GT:
            return L">";
		case LIKE:
			return L" LIKE ";
        default:
            throw new std::exception("invalid operator");
        }
    }

private:
    std::wstring propName_;
    ValueType value_;
    QueryOp op_;
};

template<typename ValueType>
std::wstring QueryObject<ValueType>::GetExpression() const{
    std::wstring sql(propName_);
    sql.append(GetOperator());
    sql.append(L"?");
    return sql;
}

template<>
void  QueryObject<int>::Bind(sqlite_command& cmd, int& index){
	cmd.bind_int(index++, value_);
}

template<>
void QueryObject<const wchar_t*>::Bind(sqlite_command& cmd, int& index){
	cmd.bind_wstr(index++, value_);
}

template<>
void QueryObject<std::wstring>::Bind(sqlite_command& cmd , int& index){
	cmd.bind_wstr(index++, value_.c_str());
}

template<>
void QueryObject<std::string>::Bind(sqlite_command& cmd, int& index){
	cmd.bind_str(index++, value_.c_str());
}

template<>
void QueryObject<const char*>::Bind(sqlite_command& cmd, int& index){
	cmd.bind_str(index++, value_);
}

template<>
void QueryObject<bool>::Bind(sqlite_command& cmd, int& index){
	cmd.bind_int(index++, value_ ? 1 : 0);
}

template<>
void QueryObject<__int64>::Bind(sqlite_command& cmd, int& index){
	cmd.bind_int64(index++, value_);
}

class Property {
public:
    explicit Property(const std::wstring& name): name_(name){};
    std::wstring GetName() const{
        return name_;
    }
private:
    std::wstring name_;
};

template<typename T>
std::shared_ptr<QueryExpression> operator==(const Property& prop, T val){
    return std::shared_ptr<QueryExpression>(new QueryObject<T>(prop.GetName(), QueryOp::EQ, val));
}
template<typename T>
std::shared_ptr<QueryExpression> operator!=(const Property& prop, T val){
    return std::shared_ptr<QueryExpression>(new QueryObject<T>(prop.GetName(), QueryOp::NEQ, val));
}
template<typename T>
std::shared_ptr<QueryExpression> operator>=(const Property& prop, T val){
    return std::shared_ptr<QueryExpression>(new QueryObject<T>(prop.GetName(), QueryOp::GE, val));
}
template<typename T>
std::shared_ptr<QueryExpression> operator<=(const Property& prop, T val){
    return std::shared_ptr<QueryExpression>(new QueryObject<T>(prop.GetName(), QueryOp::LE, val));
}
template<typename T>
std::shared_ptr<QueryExpression> operator>(const Property& prop, T val){
    return std::shared_ptr<QueryExpression>(new QueryObject<T>(prop.GetName(), QueryOp::GT, val));
}
template<typename T>
std::shared_ptr<QueryExpression> operator<(const Property& prop, T val){
    return std::shared_ptr<QueryExpression>(new QueryObject<T>(prop.GetName(), QueryOp::LT, val));
}
template<typename T>
std::shared_ptr<QueryExpression> operator %(const Property& prop, T val);

template<>
std::shared_ptr<QueryExpression> operator %(const Property& prop, std::wstring& val){
	return std::shared_ptr<QueryExpression>(new QueryObject<std::wstring>(prop.GetName(), QueryOp::LT, val));
}
template<>
std::shared_ptr<QueryExpression> operator %(const Property& prop, const wchar_t* val){
	return std::shared_ptr<QueryExpression>(new QueryObject<const wchar_t*>(prop.GetName(), QueryOp::LIKE, val));
}
template<>
std::shared_ptr<QueryExpression> operator %(const Property& prop, const char* val){
	return std::shared_ptr<QueryExpression>(new QueryObject<const char*>(prop.GetName(), QueryOp::LIKE, val));
}
template<>
std::shared_ptr<QueryExpression> operator %(const Property& prop, std::string& val){
	return std::shared_ptr<QueryExpression>(new QueryObject<std::string>(prop.GetName(), QueryOp::LIKE, val));
}

std::shared_ptr<QueryExpression> operator &&(std::shared_ptr<QueryExpression> left, std::shared_ptr<QueryExpression> right){
    return std::shared_ptr<QueryExpression>(new BinaryExpression(left, true, right));
}
std::shared_ptr<QueryExpression> operator ||(std::shared_ptr<QueryExpression> left, std::shared_ptr<QueryExpression> right){
    return std::shared_ptr<QueryExpression>(new BinaryExpression(left, false, right));
}
std::shared_ptr<QueryExpression> operator !(std::shared_ptr<QueryExpression> op){
    return std::shared_ptr<QueryExpression>(new NotExpression(op));
}
