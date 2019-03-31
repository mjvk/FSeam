// MIT License
//
// Copyright (c) 2019 Quentin Balland
// Project : https://github.com/FreeYourSoul/FSeam
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

//
// Created by FyS on 27/06/17.
//

#ifndef FREESOULS_MOCKVERIFIER_HH
#define FREESOULS_MOCKVERIFIER_HH

#include <utility>
#include <string>
#include <functional>
#include <memory>
#include <iostream>
#include <variant>
#include <map>
#include <any>

namespace FSeam {


    /**
     * @brief TypeTraitsClass with a ClassName alias in order to get the name of the class to mock (used as key for the methods)
     * @note The typetraits specifications are generated by FSeam generator
     * @tparam T type to mock
     */
    template <typename T>
    struct TypeParseTraits {
        inline static const std::string ClassName = "Undefined";
    };

    /**
     * @brief Called Comparators option used in verify in order to give more flexibility into the check possible via te verify option
     * @note To be used in order to check the number of time a method has been called 
     */
    struct VerifyCompare {
        explicit VerifyCompare(uint toCompare) : _toCompare(toCompare) {}
        bool compare(uint number) { return _toCompare == number; } 
        std::string expectStr(uint number) const { return std::string("we expected exactly ") +
            std::to_string(_toCompare) + std::string(" method call but received ") + std::to_string(number); };
        int _toCompare = 0;
    };
    struct NeverCalled {
        bool compare(uint number) { return !number; } 
        std::string expectStr(uint number) const { return std::string("we expected this method to never be called ") +
            std::to_string(_toCompare) + std::string(" but received ") + std::to_string(number); };
        int _toCompare = 0;
    };
    struct AtLeast {
        explicit AtLeast(uint toCompare) : _toCompare(toCompare) {}
        bool compare(uint number) const { return _toCompare <= number; }
        std::string expectStr(uint number) { return std::string("we expected at least ") +
            std::to_string(_toCompare) + std::string(" method call but received ") + std::to_string(number); };
        uint _toCompare = 0;
    };
    struct AtMost {
        explicit AtMost(uint toCompare) : _toCompare(toCompare) {}
        bool compare(uint number) const { return _toCompare >= number; }
        std::string expectStr(uint number) { return std::string("we expected at most ") +
            std::to_string(_toCompare) + std::string(" method call but received ") + std::to_string(number); };
        uint _toCompare = 0;
    };
    struct IsNot { // Todo: Improve is not to take a list of params to check agains instead of a single value
        explicit IsNot(uint toCompare) : _toCompare(toCompare) {}
        bool compare(uint number) const { return _toCompare != number; }
        std::string expectStr(uint number) { return std::string("we expected other value than ") +
            std::to_string(_toCompare) + std::string(" method call but received ") + std::to_string(number); };
        uint _toCompare = 0;
    };

    template <typename TypeTraitClass>
    struct isCalledComparator { static const bool v = false;  };
    template <> struct isCalledComparator<IsNot> { static const bool v = true; };
    template <> struct isCalledComparator<AtMost> { static const bool v = true; };
    template <> struct isCalledComparator<AtLeast> { static const bool v = true; };
    template <> struct isCalledComparator<NeverCalled> { static const bool v = true; };
    template <> struct isCalledComparator<VerifyCompare> { static const bool v = true; };

    struct StaticComparatorCaller {
        using CalledCompare = std::variant<IsNot, AtMost, AtLeast, NeverCalled, VerifyCompare>;

        static bool compare(CalledCompare &comparator, uint value) {
            if (auto varNever = std::get_if<NeverCalled>(&comparator))
                return varNever->compare(value);
            else if (auto varComp = std::get_if<VerifyCompare>(&comparator))
                return varComp->compare(value);
            else if (auto varAl = std::get_if<AtLeast>(&comparator))
                return varAl->compare(value);
            else if (auto varAm = std::get_if<AtMost>(&comparator))
                return varAm->compare(value);
            else if (auto varIn = std::get_if<IsNot>(&comparator))
                return varIn->compare(value);
            return false;
        }
    };

    /**
     * @brief Comparators option used in verify in order to give more flexibility into the check possible via te verify option
     * @note To be used in order to check the arguments of a method via the MockClassVerifier::verifyArg method
     */
    namespace comparator::internal {
        template<class X, class Y, class Op>
        struct op_valid_impl
        {
            template<class U, class L, class R>
            static auto test(int) -> decltype(std::declval<U>()(std::declval<L>(), std::declval<R>()),
                    void(), std::true_type());

            template<class U, class L, class R>
            static auto test(...) -> std::false_type;

            using type = decltype(test<Op, X, Y>(0));

        };
        template<class X, class Op> using op_valid = typename op_valid_impl<X, X, Op>::type;

        template<class X> using has_equality = op_valid<X, std::equal_to<>>;
        template<class X> using has_inequality = op_valid<X, std::not_equal_to<>>;

        struct Any {
        };

        struct Eq {
            Eq(std::any toCompare) : _toCompare(std::move(toCompare)) {}

            template<typename TypeToCompare>
            bool compare(TypeToCompare value) const { return value == std::any_cast<TypeToCompare>(_toCompare); }

            std::any _toCompare;
        };

        struct NotEq {
            NotEq(std::any toCompare) : _toCompare(std::move(toCompare)) {}

            template<typename TypeToCompare>
            bool compare(TypeToCompare value) const { return value != std::any_cast<TypeToCompare>(_toCompare); }

            std::any _toCompare;
        };

        struct CustomComparator {
            CustomComparator(std::any predicate) : _comparePredicate(predicate) {}

            template<typename TypeToCompare>
            bool compare(TypeToCompare value) const {
                bool ok = std::invoke(std::any_cast<std::function<bool(std::decay_t<TypeToCompare>)> >(_comparePredicate), (std::forward<TypeToCompare>(value)));
                return ok;
            }

            std::any _comparePredicate;
        };

        using ArgComparatorType = std::variant<CustomComparator, NotEq, Eq, Any>;
    }

    struct ArgComp {
        ArgComp(comparator::internal::ArgComparatorType && comp) : _comp(std::move(comp)) {}

        template <typename TypeToCompare>
        bool compare(TypeToCompare value) const {
            if (std::get_if<comparator::internal::Any>(&_comp))
                return true;
            else if (auto varCustom = std::get_if<comparator::internal::CustomComparator>(&_comp))
                return varCustom->compare<TypeToCompare>(value);

            if constexpr (comparator::internal::has_equality<TypeToCompare>()) {
                if (auto varEq = std::get_if<comparator::internal::Eq>(&_comp))
                    return varEq->compare<TypeToCompare>(value);
            }
            if constexpr (comparator::internal::has_inequality<TypeToCompare>()) {
                if (auto varNotEq = std::get_if<comparator::internal::NotEq>(&_comp))
                    return varNotEq->compare<TypeToCompare>(value);
            }
            return false;
        }
        comparator::internal::ArgComparatorType _comp;
    };
    static ArgComp Any() {
        return ArgComp(comparator::internal::Any());
    }
    template <typename T>
    static ArgComp Eq(T && t) {
        return ArgComp(comparator::internal::Eq(std::forward<T>(t)));
    }
    template <typename T>
    static ArgComp NotEq(T && t) {
        return ArgComp(comparator::internal::NotEq(std::forward<T>(t)));
    }
    template <typename T>
    static ArgComp CustomComparator(std::function<bool (std::decay_t<T>)> && t) {
        return ArgComp(comparator::internal::CustomComparator(std::forward<std::function<bool (std::decay_t<T>)>>(t)));
    }

    /**
     * @brief basic structure that contains description and usage metadata of a mocked method
     */
    struct MethodCallVerifier {
        struct Expectation  { 
            bool operator()() { return StaticComparatorCaller::compare(_comparator, _numberTimeMatched); }
            void check(void *data) {
                if (_expectator(data))
                    ++_numberTimeMatched;
            }
            std::function<bool(void*)> _expectator;
            StaticComparatorCaller::CalledCompare _comparator;
            uint _numberTimeMatched = 0;
        };

        std::string _methodName;
        std::size_t _called = 0;
        std::function<void(void*)> _handler;  
        std::vector<Expectation> _expectations;      
    };

    /**
     * @brief Mocking class, it contains all mocked method / save all calls to methods
     * @details A mock verifier instance class is a class that acknowledge all utilisation (method calls) of the mocked class
     *          this class also contains the mocked method (dupped).
     * @todo improve the mocking class to take the arguments and compare them in a verify
     */
    class MockClassVerifier {
    public:
        explicit MockClassVerifier(std::string className) : _className(std::move(className)) {}

        /**
         * @note This method should never be used by the client directly, it is a "FSeam generated" method only
         */
        void invokeDupedMethod(const std::string &methodName, void *arg = nullptr) {
            std::string key = _className + methodName;

            if (_verifiers.find(key) != _verifiers.end()) {
                if (auto dupedMethod = _verifiers.at(key)->_handler; dupedMethod)
                    dupedMethod(arg);
            }
        }

        /**
         * @note This method should never be used by the client directly, it is a "FSeam generated" method only
         */
        void methodCall(std::string methodName, void *data) {
            std::shared_ptr<MethodCallVerifier> methodCallVerifier;
            std::string key = _className + methodName;

            if (_verifiers.find(key) != _verifiers.end())
                methodCallVerifier = _verifiers.at(key);
            else 
                methodCallVerifier = std::make_shared<MethodCallVerifier>();
            for (auto &expectation : methodCallVerifier->_expectations)
                expectation.check(data);
            methodCallVerifier->_methodName = std::move(methodName);
            methodCallVerifier->_called += 1;
            _verifiers[std::move(key)] = methodCallVerifier;
        }

        /**
         * Clear the expectations of the given method, if none provided, all expectation are removed
         * @param methodName
         */
        void clearExpectations(std::optional<std::string> methodName = std::nullopt) {
            if (methodName) {
                std::string key = _className + std::move(*methodName);

                if (_verifiers.find(key) != _verifiers.end()) {
                    std::shared_ptr<MethodCallVerifier> &methodCallVerifier = _verifiers.at(key);
                    methodCallVerifier->_expectations.clear();
                }
            }
            else {
                for( auto const& [key, val] : _verifiers)
                    val->_expectations.clear();
            }
        }

        /**
         * @note This method should never be used by the client directly, it is a "FSeam generated" method only
         */
        void registerExpectation(std::string methodName, MethodCallVerifier::Expectation expectation) {
            std::shared_ptr<MethodCallVerifier> methodCallVerifier;
            std::string key = _className + std::move(methodName);

            if (_verifiers.find(key) != _verifiers.end())
                methodCallVerifier = _verifiers.at(key);
            else 
                methodCallVerifier = std::make_shared<MethodCallVerifier>();
            methodCallVerifier->_expectations.emplace_back(std::move(expectation));
            _verifiers[std::move(key)] = methodCallVerifier;
        }

        /**
         * @details Add an expectation on the specified method (template specification on a FSeam generated structure representing
         *        a specific method for a mocked class).
         *        It is important to note that a call to verify has to be done in order to check those expectations.
         * @note Working in a similar way to dupeMethod, it is required that the expectation are set before launching your executions
         *       as the library is going to check registered expectations at runtime. The verify check at the end is used in order
         *       to check if the number of occurence.
         *
         * @example
         * @code
         * class ClassName {
         * public:
         *      void functionName(int arg1, int arg2, int arg3);
         * };
         *
         * void test () {
         *      ClassName instance {};
         *      auto fseamMock = FSeam::get(&instance);
         *      // !! No execution here, otherwise it won't be checked by the following expectArg
         *      fseamMock->expectArg<FSeam::ClassName::functionName>(FSeam::Eq{24}, FSeam::NotEq{55}, FSeam::Any{});
         *      //.... code that execute functionName at some point
         *      // verify the method has been called at least once with first arg equal 21, second not equal 55, and third anything
         *      assert(fseamMock->verify(FSeam::ClassName::functionName::NAME));
         * }
         * @endcode
         *
         * @tparam ClassMethodIdentifier identifier structure generated by FSeam which represent a specific method of a specific class
         * @tparam Verifiers could be simple type (in this case the comparator FSeam::Eq is being called) or any Comparator,
         * @param verifiers comparator used in order to check the arguments of the method identified by ClassMethodIdentifier
         * @return true if the method has been called at least once, false otherwise
         */
        template <typename ClassMethodIdentifier, typename ...Verifiers>
        void expectArg(Verifiers ... verifiers);

        /**
         * @brief Call dupeMethod in order to set the set the correct return value
         * @note The duping is done in a composed way, calling dupeReturn won't override current dupe
         * 
         * @tparam ClassMethodIdentifier identifier structure generated by FSeam which represent a specific method of a specific class
         * @tparam ReturnType Return type of the function to mock
         * @param ret return value to return when the mocked method is called
         */
        template <typename ClassMethodIdentifier, typename ReturnType>
        void dupeReturn(ReturnType ret);

        /**
         * @brief This method make it possible to dupe a method in order to have it do what you want.
         *        This is a low level function that require the user to understand how the generated data struct
         *        containing the function call metadata works (as this is the argument given to the handler)
         * @note Duping an existing method doesn't remove the expectations attached to it.
         *
         * @param className name of the class to mock
         * @param methodName method name to dupe
         * @param handler dupped method that takes the data structure pointer. This data structure is a struct
         *         that contains all the data for the called method (arguments, return values)
         * @param isComposed if true, compose a new handler with the current one and the provided one,
         *         if false, override the existing handler if any. Set at false by default
         */
        void dupeMethod(std::string methodName, const std::function<void(void*)> &handler, bool isComposed = false) {
            auto methodCallVerifier = std::make_shared<MethodCallVerifier>();
            std::string key = _className + methodName;

            if (_verifiers.find(key) != _verifiers.end())
                methodCallVerifier = _verifiers.at(key);
            methodCallVerifier->_methodName = std::move(methodName);
            if (isComposed && methodCallVerifier->_handler) {
                methodCallVerifier->_handler = [currentHandler = methodCallVerifier->_handler, handler](void *data){
                    currentHandler(data);
                    handler(data);
                };
            }
            else {
                methodCallVerifier->_called = 0;
                methodCallVerifier->_handler = handler;
            }
            _verifiers[std::move(key)] = methodCallVerifier;
        }

        /**
         * @brief Verify if the given method has been called at least one time
         * 
         * @param methodName Name of the method to check on the mock (Use the helpers constant to ensure no typo)
         * @param verbose flag if a debug string is required in case of false response (set to true by default)
         * @return true if the method encounter the provided comparator conditions, false otherwise
         */
        bool verify(const std::string &methodName, bool verbose = true) const {
            return verify(methodName, AtLeast(1), verbose);
        }

        /**
         * @brief Verify if a method has been called under certain conditions (number of times)
         * 
         * @tparam Comparator  comparator class used, can be also an Integer, those comparator are defined under the namespace FSeam::VerifierComparator, 
         *          VerifyCompare/Integral value : Check if the method has been called exactly the number provided
         *          NeverCalled : Check if the method has never been called
         *          AtLeast : Check if the method has been called at least the number provided
         *          AtMost  : Check if the method has been called at most the number provided
         *          IsNot   : Check if the method is not the number provided
         *          
         * @param methodName Name of the method to check on the mock (Use the helpers constant to ensure no typo)
         * @param comp comparator instance on which the number of times the mock method is called on a provided value
         *         is checked against
         * @param verbose flag if a debug string is required in case of false response (set to true by default)
         * @return true if the method encounter the provided comparator conditions, false otherwise
         */
        template <typename Comparator>
        bool verify(std::string methodName, Comparator &&comp, bool verbose = true) const {
            if constexpr (std::is_integral<Comparator>())
                return verify(std::move(methodName), VerifyCompare{ static_cast<uint>(comp) });
            else {
                std::string key = _className + std::move(methodName);

                if (_verifiers.find(key) == _verifiers.end()) {
                    if (verbose && comp._toCompare > 0u) {
                        std::cout << "Verify error for method " << key << ", method never have been called while "
                                  << comp.expectStr(0u) << " method call \n";
                    }
                    return comp._toCompare == 0u;
                }
                bool result = true;
                for (auto &expect : _verifiers.at(key)->_expectations)
                    result &= expect();
                result &= comp.compare(_verifiers.at(key)->_called);
//                if (verbose && !result) {
//                    std::cout << "Verify error for method " << key << ", method has been called but "
//                              << comp.expectStr(_verifiers.at(key)->_called) << " method call\n";
//                }
                return result;
            }
        }

    private:
        std::string _className;
        std::map<std::string, std::shared_ptr<MethodCallVerifier> > _verifiers;
    };

    /**
     * @brief Mocking singleton, this is the main class of FSeam class contains all the mock
     */
    class MockVerifier {
        inline static std::unique_ptr<MockVerifier> inst = nullptr;

    public:
        MockVerifier() = default;
        ~MockVerifier() = default;

        static MockVerifier &instance() {
            if (inst == nullptr) {
                inst = std::make_unique<MockVerifier>();
            };
            return *(inst.get());
        }

        /**
         * @brief Clean the FSeam context of all previously set mock behaviors
         */
        static void cleanUp() {
            inst.reset(nullptr);
        }

        bool isMockRegistered(const void *mockPtr) {
            return this->_mockedClass.find(mockPtr) != this->_mockedClass.end();
        }

        /**
         * @brief This method get the MockClassVerifier instance class
         * @details Method that retrieve the FSeam MockClassVerifier instance class corresponding to the pointer given as parameter
         *
         * @param mockPtr pointer on the instance to mock
         * @param classMockName name of the class to mock (provided by TypeParseTraits)
         * @return a MockClassVerifier shared_ptr class, if not referenced yet, create one by calling the ::addMock(T) method
         */
        std::shared_ptr<MockClassVerifier> &getMock(const void *mockPtr, const std::string &classMockName) {
            if (!isMockRegistered(mockPtr))
                return addMock(mockPtr, classMockName);
            return this->_mockedClass.at(mockPtr);
        }

        /**
         * @brief This method get the default MockClassVerifier for a class type
         * @details Method that retrieve the default MockClassVerifier instance class corresponding to type given as template parameter
         *
         * @param classMockName name of the class to mock (provided by FSeam::TypeParseTraits)
         * @return a MockClassVerifier shared_ptr class, if not referenced yet, create one by calling the ::addDefaultMock(T) method
         */
        std::shared_ptr<MockClassVerifier> &getDefaultMock(const std::string &classMockName) {
            if (this->_defaultMockedClass.find(classMockName) == this->_defaultMockedClass.end())
                return addDefaultMock(classMockName);
            return this->_defaultMockedClass.at(classMockName);
        }

    private:
        std::shared_ptr<MockClassVerifier> &addMock(const void *mockPtr, const std::string &className) {
            this->_mockedClass[mockPtr] = std::make_shared<MockClassVerifier>(className);
            return this->_mockedClass.at(mockPtr);
        }
        std::shared_ptr<MockClassVerifier> &addDefaultMock(const std::string &className) {
            this->_defaultMockedClass[className] = std::make_shared<MockClassVerifier>(className);
            return this->_defaultMockedClass.at(className);
        }

    private:
        std::map<const void*, std::shared_ptr<MockClassVerifier> > _mockedClass;
        std::map<std::string, std::shared_ptr<MockClassVerifier> > _defaultMockedClass;
    };

    // ------------------------ Helper Client Free functions -------------------------- 

    /**
     * @brief This method get the MockClassVerifier instance class
     * @details Method that retrieve the mock verifier instance class corresponding to the pointer given as parameter
     *
     * @tparam T type of the instance to mock
     * @param mockPtr pointer on the instance to mock
     * @return the mock verifier instance class, if not referenced yet, create one by calling the ::addMock(T) method
     */
    template <typename T>
    std::shared_ptr<MockClassVerifier> &get(const T *mockPtr) {
        return FSeam::MockVerifier::instance().getMock(mockPtr, TypeParseTraits<T>::ClassName);
    }

    /**
     * @brief This method get the MockClassVerifier instance for the given class type
     * @details Get the Default MockClassVerifier correspond to the templated class
     *          This method has to be used in order to set default behaviors on a class type without needing to access
     *          the actual instance you want to mock
     *
     * @tparam T Class type that is going to be default mocked
     * @return std::shared_ptr<MockClassVerifier>& 
     */
    template <typename T>
    std::shared_ptr<MockClassVerifier> &getDefault() {
        return FSeam::MockVerifier::instance().getDefaultMock(TypeParseTraits<T>::ClassName);
    }

}

#endif //FREESOULS_MOCKVERIFIER_HH
