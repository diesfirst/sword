#ifndef STATE_HPP_
#define STATE_HPP_

#include <command/commandtypes.hpp>
#include <command/commandpools.hpp>
#include <state/report.hpp>
#include <state/editstack.hpp>
#include <types/stack.hpp>
#include <types/map.hpp>
#include <functional>
#include <bitset>
#include <event/event.hpp>
#include <command/vocab.hpp>

#define STATE_BASE(name) \
    void handleEvent(Event* event) override;\
    virtual const char* getName() const override {return name;}

namespace sword { namespace state { class State; }; };

namespace sword
{

//namespace event{ class Event;}
    
namespace state
{

template <typename R>
using ReportCallbackFn = std::function<void(const R*)>;
template <typename R>
using ReportCallbacks = std::vector<ReportCallbackFn<R>>;
using OwningReportCallbackFn = std::function<void(Report*)>;
using ExitCallbackFn = std::function<void()>;
using GenericCallbackFn = std::function<void()>;
using OptionMask = std::bitset<32>;
using Option = uint8_t;
using Optional = std::optional<Option>;

template <typename T>
using Reports = std::vector<std::unique_ptr<T>>;

//constexpr bool isCommandLine(event::Event* event) { return event->getCategory() == event::Category::CommandLine;}
constexpr event::CommandLine* toCommandLine(event::Event* event) { return static_cast<event::CommandLine*>(event);}

template<typename T>
constexpr Option opcast(T op) {return static_cast<Option>(op);}

template<typename T>
constexpr T opcast(Option op) {return static_cast<T>(op);}

enum class StateType : uint8_t {leaf, branch};

//forward decs
class LoadFragShaders;
class LoadVertShaders;
class SetSpec;

struct Register
{
    LoadFragShaders* loadFragShaders;
    LoadVertShaders* loadVertShaders;
    SetSpec* setSpec;
};

struct StateArgs
{
    EditStack& es;
    CommandStack& cs;
    CommandPools& cp;
    Register& rg;
};

struct Callbacks
{
    ExitCallbackFn ex{nullptr};
    OwningReportCallbackFn rp{nullptr};
    GenericCallbackFn gn{nullptr};
};

class Vocab
{
public:
    void clear() { words.clear(); }
    void push_back(std::string word) { words.push_back(word); }
    void set( std::vector<std::string> strings) { words = strings; }
    void setMaskPtr( OptionMask* ptr ) { mask = ptr; }
    std::vector<std::string> getWords() const 
    {
        if (mask)
        {
            std::vector<std::string> wordsReturn;
            for (int i = 0; i < words.size(); i++) 
                if ((*mask)[i])
                    wordsReturn.push_back(words.at(i));
            return wordsReturn;
        }
        return words;
    }
    void print()
    {
        if (mask)
        {
            for (int i = 0; i < words.size(); i++) 
                if ((*mask)[i])
                    std::cout << words.at(i) << " ";
        }
        else
        {
            for (int i = 0; i < words.size(); i++) 
                std::cout << words.at(i) << " ";
        }
        std::cout << std::endl;
    }
private:
    OptionMask* mask{nullptr};
    std::vector<std::string> words;
};

class OptionMap
{
public:
    using Element = std::pair<std::string, Option>;
    OptionMap(std::initializer_list<Element> init) :
        map{init} 
    {
        std::sort(map.begin(), map.end(), [](const Element& e1, const Element& e2) { return e1.second < e2.second; });
    }

    std::vector<std::string> getStrings()
    {
        return map.getKeys();
    }

    std::optional<Option> findOption(const std::string& s, OptionMask mask) const
    {
        return map.findValue(s, mask);
    }
private:
    SmallMap<std::string, Option> map;
};

class State
{
public:
    virtual void handleEvent(event::Event* event) = 0;
    virtual const char* getName() const = 0;
    virtual StateType getType() const = 0;
    virtual ~State() = default;
    void onEnter();
    void onExit();
//    virtual std::vector<const Report*> getReports() const {return {};};
protected:
    State(CommandStack& cs) : cmdStack{cs} {}
    State(CommandStack& cs, ExitCallbackFn callback) : cmdStack{cs}, onExitCallback{callback} {}
    void setVocab(std::vector<std::string> strings);
    void clearVocab() { vocab.clear(); }
    void addToVocab(std::string word) { vocab.push_back(word); }
    void updateVocab();
    void printVocab();
    void setVocabMask(OptionMask* mask) { vocab.setMaskPtr(mask); }
    std::vector<std::string> getVocab();
private:
    CommandPool<command::UpdateVocab> uvPool{1};
    CommandPool<command::PopVocab> pvPool{1};
    CommandPool<command::AddVocab> avPool{1};
    Vocab vocab;
    CommandStack& cmdStack;
    ExitCallbackFn onExitCallback{nullptr};
    void pushCmd(CmdPtr ptr);
    virtual void onEnterImp();
    virtual void onExitImp();
    virtual void onEnterExt() {}
    virtual void onExitExt() {}
};

//TODO: need to delete copy assignment and copy constructors
class LeafState : public State
{
public:
    ~LeafState() = default; //may not need this
    StateType getType() const override final { return StateType::leaf; }
    void addToVocab(std::string word) { State::addToVocab(word); }
    const OwningReportCallbackFn reportCallback() const { return reportCallbackFn; }
protected:
    LeafState(StateArgs sa, Callbacks cb) :
        State{sa.cs, cb.ex}, cmdStack{sa.cs}, editStack{sa.es}, reportCallbackFn{cb.rp} {}
    void popSelf() { editStack.popState(); }
    void pushCmd(CmdPtr ptr);

    template<typename R>
    R* findReport(const std::string& name, const std::vector<std::unique_ptr<R>>& reports)
    {
        for (const auto& r : reports) 
        {
            if (r->getObjectName() == name)
                return r.get();
        }
        return nullptr;
    }

private:
    CommandPool<command::SetVocab> svPool{1};
    EditStack& editStack;
    OwningReportCallbackFn reportCallbackFn{nullptr};
    CommandStack& cmdStack;

    void onExitImp() override final;
    void onEnterImp() override final;
    virtual void onEnterExt() override {}
    virtual void onExitExt() override {}

};

//TODO: make the exit callback a requirement for branch states
//      and make Director a special state that does not require it (or just make it null)
//TODO: need to delete copy assignment and copy constructors
class BranchState : public State
{
friend class Director;
public:
    ~BranchState() = default;
    StateType getType() const override final { return StateType::branch; }
    template<typename R>
    static void addReport(Report* ptr, std::vector<std::unique_ptr<R>>* derivedReports, ReportCallbackFn<R> callback = {})
    {
        auto derivedPtr = dynamic_cast<R*>(ptr); 
        if (derivedPtr)
        {
            derivedReports->emplace_back(derivedPtr);
            if (callback)
            {
                std::invoke(callback, derivedPtr);
            }
        }
        else
        {
            std::cout << "Bad report pointer passed." << std::endl;
            delete ptr;
        }
    }

protected:
    using Element = std::pair<std::string, Option>;
    BranchState(StateArgs sa, Callbacks cb, std::initializer_list<Element> ops) :
        State{sa.cs, cb.ex}, editStack{sa.es}, options{ops}
    {
        setVocab(options.getStrings());
        setVocabMask(&topMask); 
    }
    void pushState(State* state);
    Optional extractCommand(event::Event*);
    void activate(Option op) { topMask.set(op); }
    void deactivate(Option op) { topMask.reset(op); }
    void updateActiveVocab();
    void printMask();

private:
    OptionMask topMask;
    std::vector<const Report*> reports;
    OptionMap options;
    EditStack& editStack;
};

}; //state

}; //sword

#endif /* end of include guard: STATE_HPP_ */
