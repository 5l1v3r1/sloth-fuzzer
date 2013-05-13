#ifndef FUZZER_NODES_H
#define FUZZER_NODES_H

#include <cstddef>
#include <functional>
#include <vector>
#include <utility>
#include <string>
#include <map>
#include "field.h"
#include "field_mapper.h"
#include "function_nodes.h"
#include "utils.h"

class syntax_parser;

namespace grammar {
template<typename Ret, typename... Args>
class generic_node {
public:
    typedef std::function<Ret(field_mapper&, Args...)> function_type;

    generic_node(function_type function)
    : function(std::move(function))
    {
        
        
    }

    template<typename... Ts>
    Ret allocate(field_mapper &mapper, Ts&&... args) const
    {
        return function(mapper, std::forward<Ts>(args)...);
    }
private:
    function_type function;
};
    
class field_node;
typedef std::vector<field_node*> fields_list;

struct script {
    void add_field_node(field_node *f) 
    {
        fields.push_back(f);
    }

    fields_list fields;
};

// *********************************************************************
//                            Value nodes
// *********************************************************************

class value_node {
public:
    typedef std::unique_ptr< ::value_node> return_type;

    virtual ~value_node() = default;
    virtual return_type allocate(field_mapper &mapper) = 0;
};

class const_value_node : public value_node {
public:
    const_value_node(double value);
    return_type allocate(field_mapper &mapper);
private:
    double value;
};

class node_value_node : public value_node {
public:
    node_value_node(field::identifier_type id);
    return_type allocate(field_mapper &mapper);
private:
    field::identifier_type id;
};

template<typename Functor>
class value_function_node : public value_node {
public:
    value_function_node(field::identifier_type id)
    : id(id)
    {
        
    }
    
    return_type allocate(field_mapper &mapper)
    {
        return make_unique<Functor>(id);
    }
private:
    field::identifier_type id;
};

template<typename Functor>
class binary_value_function_node : public value_node {
public:
    binary_value_function_node(value_node* lhs, value_node* rhs)
    : lhs(lhs), rhs(rhs)
    {
        
    }
    
    return_type allocate(field_mapper &mapper)
    {
        return make_unique<Functor>(
            lhs->allocate(mapper),
            rhs->allocate(mapper)
        );
    }
private:
    value_node *lhs, *rhs;
};

// *********************************************************************
//                            Filler nodes
// *********************************************************************

class filler_node {
public:
    typedef field::filler_type return_type;

    virtual ~filler_node() = default;

    virtual return_type allocate(field_mapper &mapper) const = 0;
};

class const_string_node : public filler_node {
public:
    const_string_node(std::string value);
    
    return_type allocate(field_mapper &mapper) const;
private:
    std::string value;
};

class function_value_filler_node : public filler_node {
public:
    function_value_filler_node(value_node *value);
    
    return_type allocate(field_mapper &mapper) const;
private:
    value_node* value;
};

template<typename Functor>
class function_filler_node : public filler_node {
public:
    function_filler_node(field::identifier_type id)
    : id(id)
    {
        
    }
    
    return_type allocate(field_mapper &mapper) const
    {
        return std::make_shared<Functor>(id);
    }
private:
    field::identifier_type id;
};

// *********************************************************************
//                             Field nodes
// *********************************************************************

class field_node {
public:
    typedef field return_type;
    typedef field::identifier_type identifier_type;
    enum class storage {
        bytes,
        bits
    };

    virtual ~field_node() = default;
    virtual size_t max_size() const { return 0; }
    virtual size_t min_size() const { return 0; }
    virtual storage storage_type() const { return storage::bytes; }
    
    virtual return_type allocate(field_mapper &mapper) const = 0;
};

class block_field_node : public field_node {
public:
    block_field_node(filler_node *filler, size_t size, identifier_type id = field::invalid_id);
    
    return_type allocate(field_mapper &mapper) const;
    size_t max_size() const;
    size_t min_size() const;
private:
    filler_node *filler;
    size_t size;
    field::identifier_type id;
};

class bitfield_node : public field_node {
public:
    bitfield_node(filler_node *filler, size_t size, identifier_type id = field::invalid_id);
    
    return_type allocate(field_mapper &mapper) const;
    size_t max_size() const;
    size_t min_size() const;
    storage storage_type() const;
private:
    filler_node *filler;
    size_t size;
    field::identifier_type id;
};

class varblock_field_node : public field_node {
public:
    varblock_field_node(filler_node *filler, size_t min_sz, size_t max_sz, 
      identifier_type id = field::invalid_id);
    
    return_type allocate(field_mapper &mapper) const;
    size_t max_size() const;
    size_t min_size() const;
private:
    filler_node *filler;
    size_t min_sz, max_sz;
    field::identifier_type id;
};

class compound_field_node : public field_node {
public:
    compound_field_node(fields_list *fields, identifier_type id = field::invalid_id);
    
    return_type allocate(field_mapper &mapper) const;
    size_t max_size() const;
    size_t min_size() const;
private:
    fields_list *fields;
    field::identifier_type id;
};

class compound_bitfield_node : public field_node {
public:
    compound_bitfield_node(fields_list *fields, identifier_type id = field::invalid_id);
    
    return_type allocate(field_mapper &mapper) const;
    size_t max_size() const;
    size_t min_size() const;
private:
    fields_list *fields;
    field::identifier_type id;
};

class template_def_node {
public:
    template_def_node(fields_list *fields);
    
    field allocate(field_mapper &mapper, size_t min, size_t max) const;
private:
    fields_list *fields;
};

class template_field_node : public field_node {
public:
    template_field_node(template_def_node* definition, size_t min_sz, size_t max_sz);
    
    return_type allocate(field_mapper &mapper) const;
private:
    template_def_node* definition;
    size_t min_sz, max_sz;
};

} // namespace grammar

#endif // FUZZER_NODES_H
