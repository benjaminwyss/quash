#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <limits>

//NEED TO USE execvpe for creating new processes

std::string path = "";
std::string home = "";
char workingDirectory[PATH_MAX];

void printJobs();
void pipe(std::string, std::string);
//bool contains(std::string, char);

struct job {
       int isActive;
 	     int jobid;
	     int pid;
	     std::string command;
	   };

std::vector<job> jobs;

void catch_sig_child(int sig_num)
{
    pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) != -1)
    {
        int i = 0;
        for (; i < jobs.size(); i++)
        {
          if (jobs[i].pid == pid)
          {
            std::cout << "[" << jobs[i].jobid << "]" << "  " << jobs[i].pid << " finished " << jobs[i].command << ".\n";
            jobs.erase(jobs.begin() + i);
            return;
          }
        }
	return;
    }
}

int main(int argc, char **argv, char **envp)
{
  struct sigaction sa;
  sigset_t mask_set;
  sigemptyset(&mask_set);
  sa.sa_mask = mask_set;
  sa.sa_handler = &catch_sig_child;
  sigaction(SIGCHLD, &sa, NULL);


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
		      printJobs();
        }

        else if (command == "quit" || command == "exit")
        {
          exit(0);
        }

        else
        {
          bool runInBackground = false;

          //check if the first command has a &
          if (command.find("&") != std::string::npos)
          {
            command.erase(command.find("&"), 1);
            runInBackground = true;
          }

          std::string originalCommand = command;

          std::vector<std::string> argsVector;
          argsVector.push_back(command);
          while (commandStream >> command)
          {
            if (command == "&")
            {
              runInBackground = true;
            }
            else if (command.find("&") != std::string::npos)
            {
              command.erase(command.find("&"), 1);
              runInBackground = true;
              argsVector.push_back(command);
            }
            else
            {
              argsVector.push_back(command);
            }

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
            if (!runInBackground)
            {
              int status;
	      for(;;)
              {
		pid_t waitID = waitpid(pid, &status, WNOHANG | WUNTRACED);
		if (waitID == -1)
		{
		  //error exit
		  break;
		}
		else if (waitID == 0)
		{
		  //still running
		}
		else if (waitID == pid)
		{
		  if (WIFEXITED(status))
		  {
		    //good exit
		    break;
		  }
		  else if (WIFSIGNALED(status))
		  {
		    //signal exit
		    break;
		  }
		  else if (WIFSTOPPED(status))
		  {
		    //stop exit
		    break;
		  }
		  else
		  {
		    //strange exit, possible core dump
		    break;
		  }
		}
		else
		{
		  //waitpid returned something funky
		  break;
		}
              }
            }
            else
            {
              int jobNumber = 0;
              for(;;)
              {
                if (jobNumber < jobs.size())
                {
                  if (jobs[jobNumber].jobid == jobNumber)
                  {
                    jobNumber++;
                  }
                  else
                  {
                    break;
                  }
                }
                else
                {
                  break;
                }
              }
              jobs.push_back(job());
              jobs[jobs.size() - 1].jobid = jobNumber;
              jobs[jobs.size() - 1].pid = pid;
              jobs[jobs.size() - 1].command = originalCommand;

              std::cout << "[" << jobs[jobs.size() - 1].jobid << "]" << "  " << jobs[jobs.size() - 1].pid  << " running in background.\n";
            }

            for (int i = 0; i <= argsVector.size(); i++)
            {
              delete[] arg[i];
            }
            delete[] arg;
          }
        }
      }
      else
      {
        std::cin.clear();
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
	for(i = 0; i < jobs.size(); i++)
	{
		std::cout << "[" << jobs[i].jobid << "]" << "  " << jobs[i].pid << "  " << jobs[i].command << "\n";
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
