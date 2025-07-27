#include <functional>
#include <utility>

// Our State monad template:
// S is the type of the state, A is the type of the value produced.
template<typename S, typename A>
class State
{
  public:
    // We store a function that takes a state S and returns a pair: (A, S)
    using StateFunctor = std::function<std::pair<A, S>(S)>;

  private:
    StateFunctor runState;

  public:
    // Constructor: given a function from state to (value, new state)
    explicit State(StateFunctor f)
      : runState(std::move(f))
    {
    }

    // Run the state action given an initial state.
    std::pair<A, S> run(S state) const { return runState(state); }

    // Unit (return): embed a value into the monad (does not change state)
    static State<S, A> unit(const A& a)
    {
        return State<S, A>(
            [a](S state) -> std::pair<A, S> { return { a, state }; });
    }

    // Bind operation (>>=): chains a state-producing function.
    // f is a function that takes an A and returns a State<S, B>
    template<typename B>
    State<S, B> bind(std::function<State<S, B>(A)> f) const
    {
        return State<S, B>([=](S state) -> std::pair<B, S> {
            auto [a, nextState] = run(state);
            return f(a).run(nextState);
        });
    }
};

// Helper function to create a state monad without having to write the template
// parameter explicitly.
template<typename S, typename A>
State<S, A>
makeState(std::function<std::pair<A, S>(S)> f)
{
    return State<S, A>(std::move(f));
}
