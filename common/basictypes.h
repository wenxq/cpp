
#ifndef COMMON_BASICTYPES_H_
#define COMMON_BASICTYPES_H_

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&);               \
    void operator=(const TypeName&)
#endif  // DISALLOW_COPY_AND_ASSIGN

// Used to explicitly mark the return value of a function as unused. If you are
// really sure you don't want to do anything with the return value of a function
// that has been marked with __attribute__((warn_unused_result)), wrap it with
// this. Example:
//
//   scoped_ptr<MyType> my_var = ...;
//   if (TakeOwnership(my_var.get()) == SUCCESS)
//     ignore_result(my_var.release());
//
template<typename T>
inline void ignore_result(const T&)
{
}

#endif  // COMMON_BASICTYPES_H_
