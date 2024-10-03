#pragma once

#include <any>
#include <functional>
#include <tuple>
#include <vector>

namespace Sari { namespace Utils {

	class AnyFunction {
	public:

		using Caller = std::function<std::any(const std::vector<std::any>&)>;

		AnyFunction()
		{}

		AnyFunction(Caller caller) :
			caller_(std::make_shared<Caller>(std::move(caller)))
		{}

		template<typename... Args>
		std::any operator() (Args&&... args) const
		{
			std::vector<std::any> vargs = { args... };
			return (*caller_)(vargs);
		}

		explicit operator bool() const noexcept
		{
			return caller_ != nullptr;
		}

	private:

		std::shared_ptr<Caller> caller_;

	};

	template <typename T>
	class AnyFunctionWrapper;

	template <typename R, typename... A>
	class AnyFunctionWrapper<std::function<R(A...)>> {
	public:

		using FuncType = std::function<R(A...)>;
		using FuncResult = R;
		using FuncParams = std::tuple<A...>;
		static constexpr std::size_t NumOfFuncParams = std::tuple_size<FuncParams>::value;

		static AnyFunction create(const FuncType& f)
		{
			return create_<FuncResult>(f);
		}

	private:

		template<typename T>
		static auto ForwardArgument(const std::any& arg)
		{
			return std::forward<T>(std::any_cast<T>(arg));
		}

		// calls the original (wrapped) function with a return value
		template <std::size_t... I>
		static FuncResult call(const FuncType& f, const std::vector<std::any>& args, std::index_sequence<I...>)
		{
			return f(
				ForwardArgument<std::tuple_element<I, FuncParams>::type>(args[I])...
			);
		}

		// calls the original (wrapped) function with no return value
		template <std::size_t... I>
		static void callVoid(const FuncType& f, const std::vector<std::any>& args, std::index_sequence<I...>)
		{
			f(
				ForwardArgument<std::tuple_element<I, FuncParams>::type>(args[I])...
			);
		}

		// creates wrapper for a function with a return value 
		template<typename R>
		static AnyFunction create_(const FuncType& f)
		{
			auto caller = [f](const std::vector<std::any>& args) {

				if (args.size() < NumOfFuncParams) {
					throw std::bad_any_cast();
				}

				// call the captured function and converts its return value to std::any
				return std::any{
					call(f, args,  std::make_index_sequence<NumOfFuncParams>{})
				};
			};

			return AnyFunction{std::move(caller)};
		}

		// explicit specialization for wrapping a function with no return value
		template<>
		static AnyFunction create_<void>(const FuncType& f)
		{
			auto caller = [f](const std::vector<std::any>& args) {

				if (args.size() < NumOfFuncParams) {
					throw std::bad_any_cast();
				}

				// call the captured function with no return value
				callVoid(f, args, std::make_index_sequence<NumOfFuncParams>{});
				// return empty std::any value
				return std::any{};
			};

			return AnyFunction{std::move(caller)};
		}

	};

	// Wraps the input function into Utils::AnyFunction object,
	// enabling type erasure and polymorphic behavior.
	template<typename F>
	static AnyFunction MakeAnyFunc(const F& f)
	{
		return AnyFunctionWrapper<decltype(std::function{f})>::template create(f);
	}

}}
