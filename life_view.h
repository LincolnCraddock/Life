#include <sstream>

#include "life_model.hpp"

// int values for each action
const int UNDEFINED = 0, NOTHING = 1, STEP = 2, DISPLAY = 3, EXIT = 4, HELP = 5;

struct Command
{
  int action {};

  unsigned repeat = 1;
  std::string args {};

  Command () {}
  Command (int a)
      : action (a)
  {
  }
};

class View
{
private:
  // Represents the stream of input coming from a view.
  class ViewInputStream
  {
  public:
    ViewInputStream (View* v)
        : m_view (v)
    {
    }
    ViewInputStream&
    operator>> (Command& cmd);

  private:
    View* m_view;
  };
  // Represents the stream of output being displayed in a view.
  class ViewOutputStream
  {
    using manip_t = ViewOutputStream& (*) (ViewOutputStream&);

  public:
    template<typename T>
    ViewOutputStream&
    operator<< (const T& output);
    ViewOutputStream&
    operator<< (const char* output);
    ViewOutputStream&
    operator<< (manip_t func)
    {
      return func (*this);
    }

  private:
    void
    showContentsOfStream ();
    std::ostringstream buf;
    friend class View;
  };
  // Represents the stream of error messages being displayed in a view.
  class ViewErrorStream
  {
    using manip_t = ViewErrorStream& (*) (ViewErrorStream&);

  public:
    template<typename T>
    ViewErrorStream&
    operator<< (const T& output);
    ViewErrorStream&
    operator<< (const char* output);
    ViewErrorStream&
    operator<< (manip_t func)
    {
      return func (*this);
    }

  private:
    void
    showContentsOfStream ();
    std::ostringstream buf;
    friend class View;
  };

public:
  View (Model* model);
  // Update the view of the model.
  void
  update ();
  // Tell the user how to use the program.
  void
  help ();
  // The stream of input coming from the view. Use the extraction operator (>>)
  // to get input.
  ViewInputStream in;
  // The stream of output being displayed in the view. Use the insertion
  // operator (<<) to display output.
  ViewOutputStream out;
  // The stream of error messages being displayed in the view. Insert (<<) to
  // display an error message.
  ViewErrorStream err;
  // Displays the current sequence in the output stream. Insert (<<) into a
  // stream to display its contents.
  static ViewOutputStream&
  show (ViewOutputStream& vos);
  // Displays the current sequence in the error stream. Insert (<<) into a
  // stream to display its contents.
  static ViewErrorStream&
  show (ViewErrorStream& ves);

private:
  Model* m_model;
};