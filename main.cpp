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
int numberOfJobs = 0;
void printJobs();
void backgroundJob(std::string);
void pipe(std::string, std::string);
//bool contains(std::string, char);
struct job {
       int isActive;
 	     int jobid;
	     int pid;
	     std::string command;
	   };

std::vector<job> jobs;

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
        //NEED TO FIGURE OUT HOW TO PARSE THE '&' WITHOUT BREAKING OTHER PARSING COMMANDS, I SUCK WITH STRSTREAMS
        //LIKELY NEED TO FUNCTION OUT THIS SECTION TO PASS THE COMMAND THAT NEEDS TO BE EXECUTED FROM PIPE OR
        //BACKGROUND PROCESS AS WELL, I LEFT EXECUTION BLANK FOR THE TIME BEING IN THOSE FUNCTIONS
        // std::string holdCommand = command;
        // if(commandStream >> command && command == '&')
        // {
        //  backgroundJob(holdCommand);
        // }
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
		      printJobs();
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

void printJobs()
{


	std::cout << "[JOBID]  PID  COMMAND\n";
	int i;
	for(i = 0; i < numberOfJobs; i++)
	{
		std::cout << "[" << jobs[i].jobid << "]" << "  " << jobs[i].pid << "  " << jobs[i].command << "\n";
	}

}

void backgroundJob(std::string command)
{
  pid_t pid; // process id
  pid_t sid; // session id
  std::string fullCommand = command + " &";
  pid = fork(); // create a fork

  if(pid == 0)
  {
    sid = setsid(); // set a session id

    if(sid < 0) // if the sid is less than 0 then we the fork failed
    {
      std::cout << "The fork failed, oops...\n";
      exit(EXIT_FAILURE);
    }
    else
    {

      std::cout << "[" << numberOfJobs << "]" << "  " << getpid() << " is running in the background.\n";
      system(fullCommand.c_str()); // temporary execution to test, this will need to be a full bore execution step
      std::cout << "[" << numberOfJobs << "]" << "  " << getpid() << " finished " << command << ".\n";
      exit(0);
    }
  }
  else
  {
    jobs.push_back(job());
    jobs[numberOfJobs].jobid = numberOfJobs;
    jobs[numberOfJobs].pid = pid;
    jobs[numberOfJobs].command = command;
    numberOfJobs++;
    while(waitpid(pid, NULL, WEXITED|WNOHANG) > 0) 
    {
    }
  }
  
}

//void pipe(std::string fullCommand) // if we don't end up splitting the string through the stream use this

// Assuming we stream in arguments delimitted by white space we can use this version
void pipe(std::string leftCommand, std::string rightCommand)
{
  int fds[2]; // just like in lab...
  //std::string copyFull = fullCommand; // probably not needed
  //std::string commandLeft = strtok(copyFull, '|'); // just put the full command in here instead?
  //std::string commandRight = strtok(NULL, "\n"); // There might be a better function to use to break apart the command

  pid_t pid1;
  pid_t pid2;

  pid1 = fork();
  if(pid1 == 0)
  {
    dup2(fds[1], STDOUT_FILENO);
    close(fds[0]);
    int num = 0;
    // execute leftCommand here again, need to either make the main loop a function to pass this or find a work around
    exit(0);
  }

  pid2 = fork();
  if(pid2 == 0)
  {
    dup2(fds[0], STDIN_FILENO);
    close(fds[0]);
    int num = 0;
    //execute rightCommand here
  }
  close(fds[0]);
  close(fds[1]);

}

// bool contains(std::string s, char c)
// {
//   bool isFound = false;
//   for(int i = 0; i < s.length(); i++)
//   {
//     if(s[i] == c)
//     {
//       isFound = true;
//       break;
//     }
//   }
//   return isFound;
// }