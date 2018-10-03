//
//  QuickConfig.h
//  BigWall
//
//  Created by Andrew Wright on 21/8/18.
//

#ifndef QuickConfig_h
#define QuickConfig_h

#include "cinder/Json.h"

namespace Utils
{
    class QC
    {
    public:
        enum class FieldType
        {
            None,
            Float,
            Int,
            Bool,
            String,
            Vec2
        };
        
        template <FieldType T>
        struct FieldTrait {};
        
        struct FieldValue
        {
            FieldType            Type{FieldType::None};
            std::string          Name;
            
            union
            {
                float *          FloatValue{nullptr};
                int32_t * 	     IntValue;
                bool *           BoolValue;
                std::string *    StringValue;
                ci::vec2 *       Vec2Value;
            };
            
            FieldValue           ( ) { };
            FieldValue           ( const char * name, float       * value );
            FieldValue           ( const char * name, int32_t     * value );
            FieldValue           ( const char * name, bool        * value );
            FieldValue           ( const char * name, std::string * value );
            FieldValue           ( const char * name, ci::vec2    * value );
            
            void                 Read    ( const ci::JsonTree& tree );
            void                 Write   ( ci::JsonTree& tree );
            
            operator float       ( ) const { assert ( Type == FieldType::Float  ); return *FloatValue; };
            operator int32_t     ( ) const { assert ( Type == FieldType::Int    ); return *IntValue; };
            operator bool        ( ) const { assert ( Type == FieldType::Bool   ); return *BoolValue; };
            operator std::string ( ) const { assert ( Type == FieldType::String ); return *StringValue; };
            operator ci::vec2    ( ) const { assert ( Type == FieldType::Vec2   ); return *Vec2Value; };
        };
        
        QC                      ( ) { };
        QC                      ( const ci::fs::path& path, const std::initializer_list<FieldValue>& values );
        QC                      ( const std::initializer_list<FieldValue>& values );
        
        void                    Load ( const ci::fs::path& path );
        void                    Save ( const ci::fs::path& path );
        void                    Save ( );
        
        template <typename T>
        QC&                     Add  ( const std::string& name, T * value );
        
    protected:
        
        std::vector<FieldValue> _values;
        ci::fs::path            _path;
    };
}

template <>
struct Utils::QC::FieldTrait<Utils::QC::FieldType::Float> { using Type = float; };

template <>
struct Utils::QC::FieldTrait<Utils::QC::FieldType::Int> { using Type = int; };

template <>
struct Utils::QC::FieldTrait<Utils::QC::FieldType::Bool> { using Type = bool; };

template <>
struct Utils::QC::FieldTrait<Utils::QC::FieldType::String> { using Type = std::string; };

template <>
struct Utils::QC::FieldTrait<Utils::QC::FieldType::Vec2> { using Type = ci::vec2; };

Utils::QC::QC ( const ci::fs::path& path, const std::initializer_list<FieldValue>& values )
{
    for ( auto& v : values )
    {
        _values.push_back ( v );
    }
    
    Load ( path );
}

Utils::QC::QC ( const std::initializer_list<FieldValue>& values )
{
    for ( auto& v : values )
    {
        _values.push_back ( v );
    }
}

void Utils::QC::Load ( const ci::fs::path& path )
{
    try
    {
        _path = path;
        
        ci::JsonTree tree { ci::app::loadAsset ( path ) };
        for ( auto& n : tree )
        {
            for ( auto& v : _values )
            {
                if ( v.Name == n.getKey() )
                {
                    v.Read ( tree );
                }
            }
        }
    }catch ( const std::exception& e )
    {
        
    }
}

void Utils::QC::Save ( const ci::fs::path& path )
{
    ci::JsonTree tree;
    
    for ( auto& v : _values )
    {
        v.Write ( tree );
    }
    
    tree.write ( ci::writeFile ( ci::app::getAssetDirectories().front() / path ) );
}

void Utils::QC::Save ( )
{
    if ( !_path.empty() )
    {
        Save ( _path );
    }
}

template <typename T>
Utils::QC& Utils::QC::Add ( const std::string& name, T * value )
{
    _values.push_back ( Utils::QC::FieldValue ( name, value ) );
    return *this;
}

Utils::QC::FieldValue::FieldValue ( const char * name, float * value )
: Type ( QC::FieldType::Float )
, Name ( name )
, FloatValue ( value )
{ }

Utils::QC::FieldValue::FieldValue ( const char * name, int32_t * value )
: Type ( QC::FieldType::Int )
, Name ( name )
, IntValue ( value )
{ }

Utils::QC::FieldValue::FieldValue ( const char * name, bool * value )
: Type ( QC::FieldType::Bool )
, Name ( name )
, BoolValue ( value )
{ }

Utils::QC::FieldValue::FieldValue ( const char * name, std::string * value )
: Type ( QC::FieldType::String )
, Name ( name )
, StringValue ( value )
{ }

Utils::QC::FieldValue::FieldValue ( const char * name, ci::vec2 * value )
: Type ( QC::FieldType::Vec2 )
, Name ( name )
, Vec2Value ( value )
{ }

void Utils::QC::FieldValue::Read ( const ci::JsonTree& tree )
{
    if ( !tree.hasChild ( Name ) )
    {
        // Nevermind
    }else
    {
        switch ( Type )
        {
            case QC::FieldType::Float :
            {
                *FloatValue = tree[Name].getValue<float>();
                break;
            }
                
            case QC::FieldType::Int :
            {
                *IntValue = tree[Name].getValue<int32_t>();
                break;
            }
                
            case QC::FieldType::Bool :
            {
                *BoolValue = tree[Name].getValue<bool>();
                break;
            }
                
            case QC::FieldType::String :
            {
                *StringValue = tree[Name].getValue<std::string>();
                break;
            }
                
            case QC::FieldType::Vec2 :
            {
                Vec2Value->x = tree[Name + ".X"].getValue<float>();
                Vec2Value->y = tree[Name + ".Y"].getValue<float>();
                break;
            }
                
            default : {}
        }
    }
}

void Utils::QC::FieldValue::Write ( ci::JsonTree& tree )
{
    switch ( Type )
    {
        case QC::FieldType::Float :
        {
            tree.pushBack ( ci::JsonTree ( Name, *FloatValue ) );
            break;
        }
            
        case QC::FieldType::Int :
        {
            tree.pushBack ( ci::JsonTree ( Name, *IntValue ) );
            break;
        }
            
        case QC::FieldType::Bool :
        {
            tree.pushBack ( ci::JsonTree ( Name, *BoolValue ) );
            break;
        }
            
        case QC::FieldType::String :
        {
            tree.pushBack ( ci::JsonTree ( Name, *StringValue ) );
            break;
        }
            
        case QC::FieldType::Vec2 :
        {
            ci::JsonTree root = ci::JsonTree::makeObject(Name);
            root.pushBack ( ci::JsonTree ( "X", Vec2Value->x ) );
            root.pushBack ( ci::JsonTree ( "Y", Vec2Value->y ) );
            
            tree.pushBack ( root );
            
            break;
        }
            
        default : {} 
    }
}

#endif /* QuickConfig_h */
