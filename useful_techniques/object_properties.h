#pragma once

#include <utility>

/**
 * Utility stuff.
 */
template< std::size_t Index >
struct size_t_ : std::integral_constant< std::size_t, Index > {
};

template< typename Tag, std::size_t Base, std::size_t Tail >
constexpr size_t_< Tail > counter_reminder( Tag, size_t_< Base >, size_t_< Tail > ) {
    return {};
}

/**
 * Макросы для чтение значения счетчика времени компиляции.
 * Каждый счетчик характеризуется собственным тегом.
 * Максимальное значение счетчика определяется последним значением степени 2.
 */
#define PACK_ARGS( ... ) __VA_ARGS__

#define COUNTER_READ_BASE( Tag, Base, Tail ) \
counter_reminder( PACK_ARGS( Tag ){}, size_t_< Base >(), size_t_< Tail >() )

#define COUNTER_READ( Tag ) \
COUNTER_READ_BASE( PACK_ARGS( Tag ), 1, \
COUNTER_READ_BASE( PACK_ARGS( Tag ), 2, \
COUNTER_READ_BASE( PACK_ARGS( Tag ), 4, \
COUNTER_READ_BASE( PACK_ARGS( Tag ), 8, \
COUNTER_READ_BASE( PACK_ARGS( Tag ), 16, \
COUNTER_READ_BASE( PACK_ARGS( Tag ), 32, \
COUNTER_READ_BASE( PACK_ARGS( Tag ), 64, \
COUNTER_READ_BASE( PACK_ARGS( Tag ), 128, \
COUNTER_READ_BASE( PACK_ARGS( Tag ), 256, \
COUNTER_READ_BASE( PACK_ARGS( Tag ), 512, \
COUNTER_READ_BASE( PACK_ARGS( Tag ), 1024, \
        0 ) ) ) ) ) ) ) ) ) ) )

#define COUNTER_INC( Tag ) \
constexpr size_t_< COUNTER_READ( PACK_ARGS( Tag ) ) + 1 > \
counter_reminder( Tag&&, \
        size_t_< ( COUNTER_READ( PACK_ARGS( Tag ) ) + 1 ) & ~COUNTER_READ( PACK_ARGS( Tag ) ) >, \
        size_t_< ( COUNTER_READ( PACK_ARGS( Tag ) ) + 1 ) & COUNTER_READ( PACK_ARGS( Tag ) ) > ) \
        { return {}; }

/**
 * Макрос объявляет методы для упрощения доступа к свойствам класса.
 * Каждое свойство классифицируется тегом (любой тривиальный тип данных) и уникальной ролью (числовой идентификатор).
 * Для объявления свойства класса необходимо вызвать макрос BIND_PROPERTY (смотреть описание ниже).
 * При изменении свойства может быть вызван метод обработчик, если такой был указан.
 * Примеры использования методов для доступа к свойствам:
 *
 *     // --- получение значения по ссылке
 *     auto& value = object->property< Tag, Role >();
 *
 *     // --- задание значения свойства по тегу и роли
 *     object->setProperty< Tag, Role >( value );
 *
 *     // --- поиск нужной роли среди всех объявленных свойств
 *     bool state = object->toValue< Tag >( role, value );
 *     bool state = object->fromValue< Tag >( role, value );
 *
 * В последнем примере удобно использовать в качестве значения функтор с
 * переопределенными операторами приведения к типу и присваивания для шаблонных параметров.
 * Тогда нет необходимости следить за типом объекта у свойства.
 */
#define BIND_OBJECT( Object ) \
private: \
    template< typename Tag, std::size_t Role > struct PropertyBinding; \
    template< typename Tag, std::size_t Ident > struct PropertyIdentity; \
    template< typename Tag, std::size_t Role > using ObjectType = typename PropertyBinding< Tag, Role >::ObjectType; \
    template< typename Tag, std::size_t Role > using ValueType = typename PropertyBinding< Tag, Role >::ValueType; \
    \
    Object( const Object& ) = delete; \
    Object( Object&& ) = delete; \
    \
public: \
    template< typename Tag, std::size_t Role > \
    auto property() const -> const ValueType< Tag, Role >& { \
        auto& that = static_cast< ObjectType< Tag, Role >& >( const_cast< Object& >( *this ) ); \
        return PropertyBinding< Tag, Role >::value( that ); \
    } \
    \
    template< typename Tag, std::size_t Role > \
    auto property() -> ValueType< Tag, Role >& { \
        auto& that = static_cast< ObjectType< Tag, Role >& >( *this ); \
        return PropertyBinding< Tag, Role >::value( that ); \
    } \
    \
    template< typename Tag, std::size_t Role > \
    void resetProperty() { \
        auto& that = static_cast< ObjectType< Tag, Role >& >( *this ); \
        PropertyBinding< Tag, Role >::value( that ) = {}; \
        PropertyBinding< Tag, Role >::notify( that ); \
    } \
    \
    template< typename Tag, std::size_t Role > \
    void setProperty( const ValueType< Tag, Role >& value ) { \
        auto& that = static_cast< ObjectType< Tag, Role >& >( *this ); \
        PropertyBinding< Tag, Role >::value( that ) = value; \
        PropertyBinding< Tag, Role >::notify( that ); \
    } \
    \
    template< typename Tag, std::size_t Role > \
    void setProperty( ValueType< Tag, Role >&& value ) { \
        auto& that = static_cast< ObjectType< Tag, Role >& >( *this ); \
        PropertyBinding< Tag, Role >::value( that ) = std::forward< ValueType< Tag, Role > >( value ); \
        PropertyBinding< Tag, Role >::notify( that ); \
    } \
    \
    template< typename Tag, std::size_t Role, typename ...Args > \
    void setProperty( Args&& ...args ) { \
        auto& that = static_cast< ObjectType< Tag, Role >& >( *this ); \
        PropertyBinding< Tag, Role >::value( that ) = ValueType< Tag, Role >{ args... }; \
        PropertyBinding< Tag, Role >::notify( that ); \
    } \
    \
public: \
    template< typename Tag, typename ValueHolder > \
    bool toValue( std::size_t role, ValueHolder& holder ) { \
        ToValueSelector< ValueHolder > selector{ role, holder }; \
        return select< Tag >( selector, size_t_< 0 >{}, size_t_< COUNTER_READ( Tag ) - 1 >{} ); \
    } \
    \
    template< typename Tag, typename ValueHolder > \
    bool fromValue( std::size_t role, const ValueHolder& holder ) { \
        FromValueSelector< ValueHolder > selector{ role, holder }; \
        return select< Tag >( selector, size_t_< 0 >{}, size_t_< COUNTER_READ( Tag ) - 1 >{} ); \
    } \
    \
private: \
    template< typename ValueHolder > \
    struct ToValueSelector { \
        std::size_t role ; \
        ValueHolder& holder; \
        \
        template< typename Tag, std::size_t Role > \
        bool operator()( Tag, size_t_< Role >, const Object& object ) { \
            if ( role == Role ) { \
                holder = object.property< Tag, Role >(); \
                return true; \
            } \
            return false; \
        } \
    }; \
    \
    template< typename ValueHolder > \
    struct FromValueSelector { \
        std::size_t role; \
        const ValueHolder& holder; \
        \
        template< typename Tag, std::size_t Role > \
        bool operator()( Tag, size_t_< Role >, Object& object ) { \
            if ( role == Role ) { \
                ValueType< Tag, Role > value = holder; \
                object.setProperty< Tag, Role >( std::move( value ) ); \
                return true; \
            } \
            return false; \
        } \
    }; \
    \
    template< typename Tag, typename ValueSelector > \
    bool select( ValueSelector&, size_t_< 0 >, size_t_< std::size_t( -1 ) > ) { \
        return false; \
    } \
    \
    template< typename Tag, typename ValueSelector, std::size_t Index > \
    bool select( ValueSelector& selector, size_t_< Index >, size_t_< Index > ) { \
        constexpr std::size_t Role = PropertyIdentity< Tag, Index >::BindingRole; \
        return selector( Tag{}, size_t_< Role >{}, *this ); \
    } \
    \
    template< typename Tag, typename ValueSelector, std::size_t Begin, std::size_t End > \
    bool select( ValueSelector& selector, size_t_< Begin >, size_t_< End > ) { \
        constexpr std::size_t Middle = ( Begin + End ) / 2; \
        if ( select< Tag >( selector, size_t_< Begin >{}, size_t_< Middle >{} ) ) \
            return true; \
        return select< Tag >( selector, size_t_< Middle + 1 >{}, size_t_< End >{} ); \
    } \
    \
    template< typename Handler, typename ...Tail > \
    void notify( Handler&& handler, Tail&& ...tail ) { \
       if ( !handler( *this ) ) \
           return; \
       notify( tail... ); \
    } \
    \
    void notify() {}

/**
 * Объявление идентификатора свойства класса.
 * Необходим для механизма интроспекции свойств объекта.
 */
#define BIND_IDENTITY( Tag, Role, Object ) \
template<> \
struct Object::PropertyIdentity< PACK_ARGS( Tag ), COUNTER_READ( PACK_ARGS( Tag ) ) > { \
  static constexpr std::size_t BindingRole = Role; \
}; \
COUNTER_INC( PACK_ARGS( Tag ) )

/**
 * Объявление свойства класса с привязкой к указанной роли и тегу.
 * Тег характеризует группу свойств, связанных определенным признаком.
 * Роль является уникальным для группы свойств числовым идентификатором.
 * Объектом свойства, может быть любой наследник класса, помеченного макросом BIND_OBJECT.
 * Значение свойства задает путь до атрибута класса (возможно, приватного)
 * или функции, возвращающей ссылку на этот атрибут.
 * Методы оброботчики изменения свойства перечисляются в конце списка через запятую.
 * Сигнатура обработчика должна удовлетворять виду: bool( *Handler )( Object& ).
 * Если обработчик возвращает false, цепочка выполнения прерывается.
 */
#define BIND_PROPERTY( Tag, Role, Object, Value, ... ) \
template<> \
struct Object::PropertyBinding< Tag, Role > { \
    using ObjectType = Object; \
    using ValueType = std::remove_reference< decltype( std::declval< Object >().Value ) >::type; \
    static ValueType& value( ObjectType& object ) { return object.Value; } \
    static void notify( ObjectType& object ) { object.notify( __VA_ARGS__ ); } \
}; \
BIND_IDENTITY( PACK_ARGS( Tag ), Role, Object )
