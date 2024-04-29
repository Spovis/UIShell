// Kenny Kline
// Assignment 4

#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <cstdlib>
#include <regex>

using namespace std;

int makearg(char *inputStr, char ***args);

// Manually keep track of the environment variables
// this will be initialized with the PATH variable
// ie environment = {"PATH=/bin/", nullptr}
const char ** environment = (const char **)malloc(2 * sizeof(const char *));

void setEnviron(string varName, string varValue);
string getEnviron(string varName);
pid_t runWithPath(char ** args, int numWords);

int main()
{
  environment[0] = "PATH=/bin/";
  environment[1] = nullptr;

  // get a command from stdin and then use fork and exec to run the command
  while (true)
  {
    // get the command from stdin
    string command;
    cout << " $ ";
    getline(cin, command);

    // if the command is defining a variable, store the variable
    // eg ABC=thing
    if (command.find('=') != string::npos)
    {
      string varName = command.substr(0, command.find('='));
      string varValue = command.substr(command.find('=') + 1);
      setEnviron(varName.c_str(), varValue.c_str());
      continue;
    }

    // if the command has an environment variable, replace the variable with its value
    // eg 'echo $ABC' will become 'echo thing'
    size_t found = command.find('$');
    while (found != string::npos)
    {
      // use regex to find the environment variable
      regex envRegex("\\$([a-zA-Z_][a-zA-Z0-9_]*)");
      smatch match;
      regex_search(command, match, envRegex);
      string envVar = match[1];
      // replace the environment variable with its value
      string envValue;
      envValue = getEnviron(envVar.c_str());
      if (envValue.empty())
      {
        envValue = (char *)malloc(1);
        envValue[0] = '\0';
      }

      command.replace(match.position(), match.length(), envValue);
      found = command.find('$');
    }

    // if the command ends with a '&' char, execute the command in the background
    bool background = false;
    if (command[command.length() - 1] == '&')
    {
      background = true;
      command = command.substr(0, command.length() - 2);
    }

    char **args;
    int numWords = makearg((char *)command.c_str(), &args);
    if (numWords == -1)
    {
      continue;
    }

    // fork

    pid_t pid;
    pid = runWithPath(args, numWords);

    // wait for the child process to finish
    if (!background)
    {
      int status;
      waitpid(pid, &status, 0);
    }
  }

  return 0;
}

// sets a variable in our custom environment
void setEnviron(string varName, string varValue)
{
  string temp = varName + "=" + varValue;
  // allocate space for the new variable so that it can be stored in the environment
  char* cstr = new char[temp.length() + 1];
  strcpy(cstr, temp.c_str());
  int i;
  for (i = 0; environment[i] != nullptr; i++)
  {
    string envVar = environment[i];
    // if the variable already exists, replace it
    if (envVar.find(varName) != string::npos)
    {
      environment[i] = cstr;
      return;
    }
  }
  // if the variable doesn't exist, add it to the end
  // allocate space for the new variable
  environment = (const char **)realloc(environment, (i + 2) * sizeof(const char *));
  environment[i] = cstr;
  environment[i + 1] = nullptr;
  return;
}

string getEnviron(string varName)
{
  for (int i = 0; environment[i] != nullptr; i++)
  {
    const char * envVarTemp = environment[i];
    string envVar = envVarTemp;
    if (envVar.find(varName) != string::npos)
    {
      return envVar.substr(envVar.find('=') + 1);
    }
  }
  return "";
}

// run the given command with the given arguments.
// try every option in the PATH enviornment variable until one works
// in the parent process, return the pid of the child process
pid_t runWithPath(char ** args, int numWords) {
  // get the PATH environment variable

  string path = getEnviron("PATH");
  char * pathCopy = (char *)malloc(path.length() + 1);
  strcpy(pathCopy, path.c_str());
  // split it into its components
  char * pathToken = strtok(pathCopy, ":");
  // try each component until one works
  while (pathToken != nullptr)
  {
    char * pathCommand = (char *)malloc(strlen(pathToken) + strlen(args[0]) + 2);
    strcpy(pathCommand, pathToken);
    strcat(pathCommand, args[0]);
    // when we find the right one, fork and exec
    if (access(pathCommand, X_OK) == 0)
    {
      pid_t pid = fork();
      if (pid == -1)
      {
        cout << "Error: fork failed" << endl;
        return -1;
      }
      if (pid == 0)
      {
        execv(pathCommand, args);
        // if we get here, exec failed, so we need to try the next path option
      }
      free(pathCommand);
      free(pathCopy);
      // return the pid of the child process
      return pid;
    }
    free(pathCommand);
    pathToken = strtok(nullptr, ":");
  }

  // if none work, return -1 with a helpful error message
  cout << "Error: command not found for " << args[0] << endl;
  return -1;

}

/*
  Takes in a c-style string and char pointer pointer pointer.
  It should break the input string into words, and store the words in an array
  of strings.  It should return the number of words (i.e., the number of strings
  in the array), and it should allocate the array of strings.
*/
int makearg(char *inputStr, char ***args)
{
  int numWords = 0;
  char *temp = inputStr;
  bool foundCommand = false;
  bool prevIsSpace = false;
  // get the number of tokens
  while (*temp != '\0')
  {
    // don't count multiple spaces in a row as multiple tokens
    if (!isspace(*temp))
    {
      foundCommand = true;
      prevIsSpace = false;
    }
    if (isspace(*temp) && !prevIsSpace)
    {
      numWords++;
      prevIsSpace = true;
    }
    temp++;
  }
  // count the last word
  numWords++;
  // exit if the user submitted no actual input
  if (!foundCommand)
  {
    return -1;
  }
  // allocate args
  *args = (char **)malloc(numWords * sizeof(char *) + sizeof(nullptr));
  // reset temp back to the beginning of the string
  temp = inputStr;
  for (int i = 0; i < numWords; i++)
  {
    // get to the start of the next word
    while (*temp == ' ')
    {
      temp++;
    }
    // get the length of this word
    int j = 0;
    while (temp[j] != ' ' && temp[j] != '\0')
    {
      j++;
    }
    // allocate space for the word
    (*args)[i] = (char *)malloc(j + 1);
    // copy the word into the args array
    for (int k = 0; k < j; k++)
    {
      (*args)[i][k] = temp[k];
    }
    // null terminate
    (*args)[i][j] = '\0';
    // move on to the next word
    temp += j;
  }
  // null terminate the args array
  (*args)[numWords] = nullptr;
  return numWords;
}