// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "TargetBuilder.h"

#include <coroutine>
#include <exception>
#include <string>
#include <utility>

#define TWOFOLD_EXPR(expr) (::TwofoldRuntime::expressionToString([&]() -> decltype(auto) { return (expr); }))

namespace TwofoldRuntime {

template<class T>
concept IsToStringable = requires(T a) {
    { std::to_string(a) } -> std::convertible_to<std::string>;
};

template<class Expression>
auto expressionToString(Expression&& expression) -> std::string {
    if constexpr (std::is_void_v<std::invoke_result_t<Expression>>) {
        std::forward<Expression>(expression)();
        return {};
    }
    else {
        return []<class T>(T&& value) {
            if constexpr (IsToStringable<T>) {
                return std::to_string(std::forward<T>(value));
            }
            else {
                return std::string(std::forward<T>(value));
            }
        }(std::forward<Expression>(expression)());
    }
}

template<class Promise = void>
struct UniqueCoroutineHandle final {
    using Handle = std::coroutine_handle<Promise>;

    UniqueCoroutineHandle() = default;
    explicit UniqueCoroutineHandle(Handle h) noexcept
        : m(h) {}
    UniqueCoroutineHandle(UniqueCoroutineHandle const&) = delete;
    UniqueCoroutineHandle& operator=(UniqueCoroutineHandle const&) = delete;
    UniqueCoroutineHandle(UniqueCoroutineHandle&& o) noexcept
        : m(std::exchange(o.m, nullptr)) {}
    UniqueCoroutineHandle& operator=(UniqueCoroutineHandle&& o) noexcept {
        destroy(m);
        m = std::exchange(o.m, nullptr);
        return *this;
    }
    ~UniqueCoroutineHandle() noexcept { destroy(m); }

    explicit operator bool() const noexcept { return static_cast<bool>(m); }
    operator Handle() noexcept { return m; }

    // auto extract() -> Handle { return std::exchange(m, nullptr); }

    void resume() const { m.resume(); }

private:
    static void destroy(Handle h) noexcept {
        if (h) h.destroy();
    }

private:
    Handle m = nullptr;
};

struct Append {
    std::string_view text;
    FilePosition originPosition;
};
struct AppendExpression {
    std::string text;
    FilePosition originPosition;
};
struct NewLine {
    FilePosition originPosition;
};
struct PushIndentation {
    std::string_view indent;
    FilePosition originPosition;
};
struct PopIndentation {};
struct IndentPart {
    std::string_view indent;
    FilePosition originPosition;
};
struct PushPartIndent {
    FilePosition originPosition;
};
struct PopPartIndent {};
struct PushCaller {
    FilePosition originPosition;
};
struct PopCaller {};

// Input origins are explicit: ordinary C++ helper calls never enter the stack.
struct SourceScope {
    char const* location{};
    TargetBuilder* builder{};
    struct Guard {
        TargetBuilder* builder{};
        Guard(TargetBuilder* builder)
            : builder(builder) {}
        Guard(Guard const&) = delete;
        Guard(Guard&& other) noexcept
            : builder(std::exchange(other.builder, nullptr)) {}
        ~Guard() {
            if (builder) builder->popSource();
        }
    };
    auto await_ready() const noexcept -> bool { return false; }
    template<class Promise>
    auto await_suspend(std::coroutine_handle<Promise> coroutine) -> bool {
        auto* target = coroutine.promise().builder;
        auto origin = target->resolveSource(location);
        if (target->pushSource(std::move(origin))) {
            builder = target;
        }
        return false;
    }
    auto await_resume() -> Guard { return Guard{builder}; }
};

struct Twofold {
    struct promise_type {
        TargetBuilder* builder{};
        std::exception_ptr exception;

        auto initial_suspend() noexcept { return std::suspend_always{}; }
        auto final_suspend() noexcept { return std::suspend_always{}; }

        void unhandled_exception() { exception = std::current_exception(); }
        void return_void() const noexcept {}

        auto get_return_object() {
            auto handle = std::coroutine_handle<promise_type>::from_promise(*this);
            return Twofold{.handle = UniqueCoroutine{handle}};
        }

        auto yield_value(PushCaller caller) {
            builder->pushCaller(std::move(caller.originPosition));
            return std::suspend_never{};
        }
        auto yield_value(PopCaller) {
            builder->popCaller();
            return std::suspend_never{};
        }
        auto yield_value(Append append) {
            builder->append(append.text, append.originPosition);
            return std::suspend_never{};
        }
        auto yield_value(AppendExpression append) {
            builder->appendExpression(append.text, append.originPosition);
            return std::suspend_never{};
        }
        auto yield_value(NewLine newLine) {
            builder->newLine(newLine.originPosition);
            return std::suspend_never{};
        }
        auto yield_value(PushIndentation pushIndentation) {
            builder->pushIndentation(pushIndentation.indent, pushIndentation.originPosition);
            return std::suspend_never{};
        }
        auto yield_value(PopIndentation) {
            builder->popIndentation();
            return std::suspend_never{};
        }
        auto yield_value(IndentPart indentPart) {
            builder->indentPart(indentPart.indent, indentPart.originPosition);
            return std::suspend_never{};
        }
        auto yield_value(PushPartIndent pushPartIndent) {
            builder->pushPartIndent(pushPartIndent.originPosition);
            return std::suspend_never{};
        }
        auto yield_value(PopPartIndent) {
            builder->popPartIndent();
            return std::suspend_never{};
        }
    };
    using Coroutine = std::coroutine_handle<promise_type>;
    using UniqueCoroutine = UniqueCoroutineHandle<promise_type>;

    auto operator co_await() noexcept {
        struct Awaiter {
            Coroutine m_coroutine = {};

            bool await_ready() const noexcept { return false; }

            bool await_suspend(Coroutine awaitingCoroutine) const noexcept {
                m_coroutine.promise().builder = awaitingCoroutine.promise().builder;
                m_coroutine.resume(); // run called generator
                return false; // resume the awaitingCoroutine
            }
            void await_resume() const {
                if (auto exception = m_coroutine.promise().exception) std::rethrow_exception(exception);
            }
        };
        return Awaiter{handle};
    }

    void run() {
        handle.resume();
        if (auto exception = static_cast<Coroutine>(handle).promise().exception) std::rethrow_exception(exception);
    }

    void setBuilder(TargetBuilder* builder) { static_cast<Coroutine>(handle).promise().builder = builder; }

    UniqueCoroutine handle{};
};

} // namespace TwofoldRuntime

using TwofoldRuntime::Twofold;
using TwofoldBuilder = TwofoldRuntime::TargetBuilder;
