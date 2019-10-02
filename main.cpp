#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>

//NEED TO USE execvpe for creating new processes

std::string path = "";
std::string home = "";
char workingDirectory[PATH_MAX];

int main(int argc, char **argv, char **envp)
{
    bool foundPath = false;
    bool foundHome = false;

    //Locate the PATH and HOME environment variables
    while (*envp)
    {
      if (!foundPath)
      {
        path = std::string(*envp);
        if (path.substr(0, 5) == "PATH=")
        {
  	      path = path.substr(5);
  	      foundPath = true;
  	    }
      }

      if (!foundHome)
      {
        home = std::string(*envp);
        if (home.substr(0, 5) == "HOME=")
        {
  	      home = home.substr(5);
  	      foundHome = true;
  	    }
      }

	    *envp++;
    }

    std::string command = "";
    while (command != "quit" && command != "exit")
    {
      getcwd(workingDirectory, sizeof(workingDirectory));
      std::cout << workingDirectory << " > ";
      std::getline(std::cin, command);
      std::stringstream commandStream(command);

      if (commandStream >> command)
      {
        if (command == "echo")
        {
          if (commandStream >> command)
          {
            if (command == "$PATH" || command == "PATH")
            {
              system("echo $PATH");
            }
            else if (command == "$HOME" || command == "HOME")
            {
              system("echo $HOME");
            }
          }
        }

        else if (command == "set")
        {
          if (commandStream >> command)
          {
            if (command.substr(0, 5) == "PATH=" || command.substr(0, 6) == "$PATH=")
            {
              std::size_t start = command.find("=") + 1;
              if (start != std::string::npos)
              {
                if (command.substr(start, 4) == "PATH" || command.substr(start, 5) == "$PATH")
                {
                  start = command.find(":");
                  if (start != std::string::npos)
                  {
                    path += command.substr(start);
                  }
                }
                else if(command.substr(command.size() - 4, 4) == "PATH" || command.substr(command.size() - 5, 5) == "$PATH")
                {
                  std::size_t end = command.find(":$PATH");
                  if (end == std::string::npos)
                  {
                    end = command.find(":PATH");
                  }
                  command = command.substr(start, (end - start));
                  path += ":" + command;
                }
                else
                {
                  path = command.substr(start);
                }

                setenv("PATH", path.c_str(), 1);
              }

            }
            if (command.substr(0, 5) == "HOME=" || command.substr(0, 6) == "$HOME=")
            {
        	    std::size_t start = command.find("=") + 1;
        	    if (start != std::string::npos)
        	    {
        	      home = command.substr(start);
        	      setenv("HOME", home.c_str(), 1);
        	    }
            }
          }
        }

        else if (command == "cd")
        {
        	if (commandStream >> command)
        	{
        	  if (chdir(command.c_str()) < 0)
            {
              fprintf(stderr, "%s: Error - Directory Not Found.\n", command.c_str());
            }
        	}
        	else
        	{
        	  if (chdir(home.c_str()) < 0)
            {
              fprintf(stderr, "%s: Error - Home Directory Not Found.\n", home.c_str());
            }
        	}
        }

        else if (command == "jobs")
        {

        }

        else if (command == "quit" || command == "exit")
        {
          exit(0);
        }

        else
        {
          std::vector<std::string> argsVector;
          argsVector.push_back(command);
          while (commandStream >> command)
          {
            argsVector.push_back(command);
          }

          char** arg = new char*[argsVector.size() + 1];

          for (int i = 0; i < argsVector.size(); i++)
          {

            arg[i] = new char[1000];

            snprintf(arg[i], 999, "%s", argsVector.at(i).c_str());

          }
          arg[argsVector.size()] = (char*)0;

          char pathenv[path.length() + sizeof("PATH=")];
          char homeenv[home.length() + sizeof("HOME=")];
          sprintf(pathenv, "PATH=%s", path.c_str());
          sprintf(homeenv, "HOME=%s", home.c_str());

          char* env[3] = {pathenv, homeenv, (char*)0};

          pid_t pid = fork();
          if (pid == 0)
          {
            if (execvpe(arg[0], arg, env) < 0)
            {
              std::cout << "command '" << arg[0] << "' not found.\n";
              exit(-1);
            }
          }
          else
          {
            wait(NULL);

            for (int i = 0; i <= argsVector.size(); i++)
            {
              delete[] arg[i];
            }
            delete[] arg;
          }
        }
      }

    }

	//int inFile = open("in.txt", O_RDONLY);
	//int outFile = open("out.txt", O_WRONLY | O_TRUNC | O_CREAT);
	//dup2(inFile, 0);
	//dup2(outFile, 1);
	//…
	//close(inFile);
	//close(outFile);


    return 0;
}
