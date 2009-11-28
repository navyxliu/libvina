/** A demonstration of pipeline processing
 *  I intend to implement a multi-language scenario to demonstrate
 *the power of pineline. This is critical because
 *Sequoia is lack of this parallel pattern.
 *
 *  First of all, I defined a natural language type system. a Person
 *has two language properties : mother language and a foreign language.
 *Ideally, given a social circle, they will try to figure out unfamiliar
 *language by transitive help. I use this behavior to simulate information
 *flow in real system. A person is modeled as an agent who has (and also
 *limited) methods to deal with a variety information. Strings of languages
 *represent information flows.
 *
 *  Using static type, doit-driven solution is easy to implement. However,
 *function objects are much more powerful than classes only consist of static
 *functions. Using variadic template, I intentatively make it. 
 **/
#include <string>

#define NUM_OF_LANG 8
#define MAX_LENGTH_OF_LANG 20

// one word represents a type of language.
const std::string hellos[NUM_OF_LANG] = {"hello",      //             
				   "hola",             //spanish      
				   "bonjour",          //french       
				   "ciao",             //italian      
				   "nihao",            //mandarin
				   "konnichiha",       //japanese
				   "hallo",            //duntch
				   "salam",            //arabic
};
const std::string names[NUM_OF_LANG] = {"henry",       //an englishman who knows spanish
				  "juan", 
				  "fangfang",
				  "mario"
				  "xliu",
				  "ichiro",
				  "max",
				  "mohanmode"
};

typedef enum {
  English,
  Spanish,
  French,
  Italian
}lang_t;

template <lang_t lang>
struct Lang{
  const static int value = lang;
};
