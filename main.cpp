#include <iostream>
#include <fcntl.h>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
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
 	     int jobid;
	     pid_t pid;
	     std::string command;
	   };

std::vector<job> jobs;

void catch_sig_child(int sig_num, siginfo_t *siginf, void *nullvar)
{
    pid_t pid;
    pid = siginf->si_pid;
    int i = 0;
    for (; i < jobs.size(); i++)
    {
      if (jobs[i].pid == pid)
      {
        std::cout << "[" << jobs[i].jobid << "]" << " " << jobs[i].pid << " finished " << jobs[i].command << ".\n";
        jobs.erase(jobs.begin() + i);
        return;
      }
    }
    return;
}

int main(int argc, char **argv, char **envp)
{
  struct sigaction sa;
  sigset_t mask_set;
  sigemptyset(&mask_set);
  sa.sa_mask = mask_set;
  sa.sa_sigaction = &catch_sig_child;
  sa.sa_flags = SA_SIGINFO;
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

      std::string originalCommand = command;

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

        else if (command == "kill")
        {
          if(commandStream >> command)
          {
            try //todo let the user specify any signal 
            {
            }
            catch (std::invalid_argument& ia)
            {
            }
            bool isKilled = false;
            for(int i = 0; i < jobs.size(); i++)
            {
              if(command == std::to_string(jobs[i].jobid))
              {
                kill(jobs[i].pid, SIGKILL);
                isKilled = true;
                break;
              }
            }
            if(isKilled == false)
            {
              std::cout << "Failed to kill the process with job id " << command << "!" << '\n';
            }
          }
          else
          {
            std::cout << "Cannot kill a process because no signum nor job id was provided.\n";
          }
        }

        else if (command == "quit" || command == "exit")
        {
          return 0;
        }

        else
        {
          bool runInBackground = false;
          bool redirectOutput = false;
          bool redirectInput = false;

          std::string outputStr = "";
          std::string inputStr = "";

          std::vector<int> pipeIndices;

          //check if the first command has a &
          if (command.find("&") != std::string::npos)
          {
            command.erase(command.find("&"), 1);
            runInBackground = true;
          }

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
            else if (command == "<")
            {
              if (commandStream >> inputStr)
              {
                redirectInput = true;
              }
              else
              {
                std::cout << "Cannot redirect input because no file was provided.\n";
              }
            }
            else if (command == ">")
            {
              if (commandStream >> outputStr)
              {
                redirectOutput = true;
              }
              else
              {
                std::cout << "Cannot redirect output because no file was provided.\n";
              }
            }
            else if (command == "|")
            {
              argsVector.push_back(command);
              pipeIndices.push_back(argsVector.size());
            }
            else
            {
              argsVector.push_back(command);
            }

          }

          char** arg = new char*[argsVector.size() + 1];

          for (int i = 0; i < argsVector.size(); i++)
          {
            if (argsVector[i] == "|")
            {
              arg[i] = (char*)0;
            }
            else
            {
              arg[i] = new char[1000];
              snprintf(arg[i], 999, "%s", argsVector.at(i).c_str());
            }

          }
          arg[argsVector.size()] = (char*)0;

          char pathenv[path.length() + sizeof("PATH=")];
          char homeenv[home.length() + sizeof("HOME=")];
          sprintf(pathenv, "PATH=%s", path.c_str());
          sprintf(homeenv, "HOME=%s", home.c_str());

          char* env[3] = {pathenv, homeenv, (char*)0};

          int* fds = nullptr;
          std::vector<pid_t> pids;

          if (pipeIndices.size() > 0)
          {
            fds = new int[pipeIndices.size() * 2];
          }
          for (int i = 0; i < pipeIndices.size(); i++)
          {
            pipe(fds + 2*i);
          }
          if (pipeIndices.size() > 0)
          {
            for (int i = 0; i <= pipeIndices.size(); i++)
            {
              pid_t pid = fork();
              if (pid == 0)
              {
              if (i == 0)
              {
                if (redirectInput)
                {
                  int inFile = open(inputStr.c_str(), O_RDONLY);
                  dup2(inFile, 0);
                  close(inFile);
                }

                dup2(fds[1], 1);

              }
              else if (i == pipeIndices.size())
              {
                if (redirectOutput)
                {
                  int outFile = open(outputStr.c_str(), O_WRONLY | O_TRUNC | O_CREAT);
                  dup2(outFile, 1);
                  close(outFile);
                }
                dup2(fds[pipeIndices.size() * 2 - 2], 0);
              }
              else
              {
                dup2(fds[2 * i + 1], 1);
                dup2(fds[2 * i - 2], 0);
              }
              for (int i = 0; i < 2 * pipeIndices.size(); i++)
              {
                close(fds[i]);
              }

                if (i == 0)
                {
                  if (execvpe(arg[0], arg, env) < 0)
                  {
                    std::cerr << "command '" << arg[0] << "' not found.\n";
                    exit(-1);
                  }
                }
                else
                {
                  if (execvpe(arg[pipeIndices[i-1]], arg + pipeIndices[i-1], env) < 0)
                  {
                    std::cerr << "command '" << arg[pipeIndices[i-1]] << "' not found.\n";
                    exit(-1);
                  }
                }

              }
              else if (pid > 0)
              {
                pids.push_back(pid);
              }

            }
          }
          else
          {
            pid_t pid = fork();
            if (pid > 0)
            {
              pids.push_back(pid);
            }
            if (pid == 0)
            {
              if (redirectInput)
              {
                int inFile = open(inputStr.c_str(), O_RDONLY);
                dup2(inFile, 0);
                close(inFile);
              }
              if (redirectOutput)
              {
                int outFile = open(outputStr.c_str(), O_WRONLY | O_TRUNC | O_CREAT);
                dup2(outFile, 1);
                close(outFile);
              }

              if (execvpe(arg[0], arg, env) < 0)
              {
                std::cout << "command '" << arg[0] << "' not found.\n";
                exit(-1);
              }
            }
          }

          if (pids[pids.size() - 1] > 0)
          {
            for (int i = 0; i < 2*pipeIndices.size(); i++)
            {
              close(fds[i]);
            }

            if (!runInBackground)
            {
              int status;
              pid_t waitID;
              do
              {
                
              } while ((waitID = waitpid(pids[pids.size() - 1], &status, WNOHANG)) == 0);

            }
            else
            {
              int jobNumber = 1;
              for(; jobNumber < jobs.size() + 1; jobNumber++)
              {
                bool jobNumberTaken = false;
                for (int i = 0; i < jobs.size(); i++)
                {
                  if (jobs[i].jobid == jobNumber)
                  {
                    jobNumberTaken = true;
                    break;
                  }
                }
                if (!jobNumberTaken)
                {
                  break;
                }
              }
              jobs.push_back(job());
              jobs[jobs.size() - 1].jobid = jobNumber;
              jobs[jobs.size() - 1].pid = pids[pids.size() - 1];
              jobs[jobs.size() - 1].command = originalCommand;

              std::cout << "[" << jobs[jobs.size() - 1].jobid << "]" << " " << jobs[jobs.size() - 1].pid  << " running in background.\n";
            }

            for (int i = 0; i <= argsVector.size(); i++)
            {
              delete[] arg[i];
            }
            delete[] arg;
            if (fds != nullptr)
            {
              delete[] fds;
            }
          }
        }
      }
      else
      {
        std::cin.clear();
      }
    }

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
