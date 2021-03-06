/*
 * Copyright 2017-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _LAUNDER_H_
#define _LAUNDER_H_

/**
 * Approximate backport from C++17 of std::launder. It should be `constexpr`
 * but that can't be done without specific support from the compiler.
 */
template <typename T>
inline T* launder(T* in)
{
#if HAS_BUILTIN(__builtin_launder) || __GNUC__ >= 7
  // The builtin has no unwanted side-effects.
  return __builtin_launder(in);
#elif __GNUC__
  // This inline assembler block declares that `in` is an input and an output,
  // so the compiler has to assume that it has been changed inside the block.
  __asm__("" : "+r"(in));
  return in;
#elif defined(_WIN32)
  // MSVC does not currently have optimizations around const members of structs.
  // _ReadWriteBarrier() will prevent compiler reordering memory accesses.
  _ReadWriteBarrier();
  return in;
#else
  static_assert(
      false, "launder is not implemented for this environment");
#endif
}

/* The standard explicitly forbids laundering these */
void launder(void*);
void launder(void const*);
void launder(void volatile*);
void launder(void const volatile*);

#endif // _LAUNDER_H_

