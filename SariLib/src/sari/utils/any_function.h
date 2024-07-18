#pragma once

#include <any>
#include <functional>
#include <tuple>
#include <vector>

namespace Sari { namespace Utils {

	using AnyFunction = std::function<std::any(const std::vector<std::any>&)>;

	template <typename T>
	class AnyFunctionWrapper;

	template <typename R, typename... A>
	class AnyFunctionWrapper<std::function<R(A...)>> {
	public:

		using FuncType = std::function<R(A...)>;
		using FuncResult = R;
		using FuncParams = std::tuple<A...>;
		static constexpr std::size_t NumOfFuncParams = std::tuple_size<FuncParams>::value;

		static AnyFunction create(FuncType f)
		{
			return create_<FuncResult>(f);
		}

	private:

		// calls the original (wrapped) function with a return value
		template <std::size_t... I>
		static FuncResult call(FuncType f, const std::vector<std::any>& args, std::index_sequence<I...>)
		{
			return std::forward<FuncType>(f)(
				std::any_cast<std::tuple_element<I, FuncParams>::type>(args[I])...
			);
		}

		// calls the original (wrapped) function with no return value
		template <std::size_t... I>
		static void callVoid(FuncType f, const std::vector<std::any>& args, std::index_sequence<I...>)
		{
			std::forward<FuncType>(f)(
				std::any_cast<std::tuple_element<I, FuncParams>::type>(args[I])...
			);
		}

		// creates wrapper for a function with a return value 
		template<typename R>
		static AnyFunction create_(FuncType f)
		{
			return [f](const std::vector<std::any>& args) {

				if (args.size() < NumOfFuncParams) {
					throw std::bad_any_cast();
				}

				// call the captured function and converts its return value to std::any
				return std::any{
					call(f, args,  std::make_index_sequence<NumOfFuncParams>{})
				};
			};
		}

		// explicit specialization for wrapping a function with no return value
		template<>
		static AnyFunction create_<void>(FuncType f)
		{
			return [f](const std::vector<std::any>& args) {

				if (args.size() < NumOfFuncParams) {
					throw std::bad_any_cast();
				}

				// call the captured function with no return value
				callVoid(f, args, std::make_index_sequence<NumOfFuncParams>{});
				// return empty std::any value
				return std::any{};
			};
		}

	};

	class VariadicFunction {
	public:

		VariadicFunction()
		{}

		VariadicFunction(const AnyFunction& func) :
			func_(func)
		{}

		template<typename... Args>
		std::any operator() (Args&&... args) const
		{
			std::vector<std::any> vargs = { args... };
			return func_(vargs);
		}

	private:

		AnyFunction func_;

	};

	template<typename... Args>
	static std::any CallAnyFunc(const AnyFunction& f, Args&&... args)
	{
		std::vector<std::any> vargs = { args... };
		return f(vargs);
	}

	// Wraps the input function into Utils::AnyFunction object,
	// enabling type erasure and polymorphic behavior.
	template<typename F>
	static AnyFunction MakeAnyFunc(F f)
	{
		return AnyFunctionWrapper<decltype(std::function{f})>::template create(f);
	}

}}
