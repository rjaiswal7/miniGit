#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <map>

#include "sha256.h"

using namespace std;
namespace fs = std::filesystem;

// initializing a repository
void initRepository()
{
    fs::path gitDir = ".mygit";
    fs::path headFilePath = gitDir / "HEAD";
    if (fs::exists(".mygit"))
    {
        cerr << "Repository already exists\n";
        return;
    }

    fs::create_directories(gitDir / "objects");
    fs::create_directories(gitDir / "refs");
    ofstream headFile(headFilePath);
    if (!headFile)
    {
        cerr << "Failed to create HEAD\n";
        return;
    }

    headFile << "ref: refs/heads/main\n";
    headFile.close();

    cout << "Initialized empty repository\n";
}

void setConfig(const string &author)
{

    if (!fs::exists(".mygit"))
    {
        throw runtime_error("MiniGit repository is not initialized");
    }

    ofstream configFile(".mygit/config");
    if (!configFile.is_open())
    {
        throw runtime_error("Failed to open the config file\n");
    }
    configFile << "user.name=" << author << '\n';
}

string getAuthor()
{
    ifstream configFile(".mygit/config");
    if (!configFile.is_open())
    {
        return "";
    }
    string line;
    while (getline(configFile, line))
    {
        if (line.rfind("user.name=", 0) == 0)
        {
            return line.substr(10);
        }
    }
    return "";
}

// function for reading a text file
string readFile(const fs::path &filePath)
{
    ifstream file(filePath);
    if (!file.is_open())
    {
        throw runtime_error("Failed to read the file");
    }
    stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}

// creating blob from file contents
string createBlobData(const string &contents)
{
    return "blob " + to_string(contents.size()) + '\0' + contents;
}

// saving the object
void storeObject(const string &hash, const string &data)
{
    if (hash.size() != 64)
    {
        cerr << "Invalid SHA-256 hash\n";
        return;
    }
    string dir_name(hash.begin(), hash.begin() + 2);
    string file_name(hash.begin() + 2, hash.end());
    fs::path objDir = ".mygit/objects";
    fs::create_directories(objDir / dir_name);
    ofstream objFile(objDir / dir_name / file_name);
    if (!objFile.is_open())
    {
        cerr << "Failed to create object file\n";
        return;
    }
    objFile << data;
    objFile.close();
}

// loading index of files in staging are from INDEX file
map<string, string> loadIndex()
{
    map<string, string> indexMap;
    fs::path indexPath = ".mygit/index";
    ifstream indexFile(indexPath);
    if (!indexFile)
    {
        return {};
    }
    string line;
    char delimiter = '\t';
    while (getline(indexFile, line))
    {
        stringstream ss(line);
        string filePath;
        string blobHash;
        getline(ss, filePath, delimiter);
        getline(ss, blobHash);
        indexMap[filePath] = blobHash;
    }
    indexFile.close();
    return indexMap;
}

// saving the index to INDEX file
void saveIndex(const map<string, string> &indexMap)
{
    fs::path indexPath = ".mygit/index";
    ofstream indexFile(indexPath);
    if (!indexFile.is_open())
    {
        cerr << "Failed to open index file\n";
        return;
    }
    for (const auto &[filePath, blobHash] : indexMap)
    {
        indexFile << filePath << '\t' << blobHash << '\n';
    }
    indexFile.close();
}

// adding file to staging area and upadting the index
void addFile(const fs::path &filepath)
{
    string contents = readFile(filepath);
    string blob_data = createBlobData(contents);
    string blob_hash = sha256(blob_data);
    storeObject(blob_hash, blob_data);
    map<string, string> indexMap = loadIndex();
    indexMap[filepath.generic_string()] = blob_hash;
    saveIndex(indexMap);
    cout << "SHA-256 of file: " << blob_hash << '\n';
}

// generating tree data from the index
string createTreeData(const map<string, string> &indexMap)
{
    string treeData = "";
    for (const auto &[filePath, blobHash] : indexMap)
    {
        treeData += filePath + '\t' + blobHash + '\n';
    }
    return treeData;
}

// creating tree hash
string createTree(const map<string, string> &indexMap)
{
    string treeData = createTreeData(indexMap);
    string treeDataHash = sha256(treeData);
    storeObject(treeDataHash, treeData);
    return treeDataHash;
}

// generating commit data
string createCommitData(const string &treeHash, const string &parentHash, const string &author, const string &message)
{
    string commitData;
    commitData += "tree " + treeHash + '\n';
    if (!parentHash.empty())
    {
        commitData += "parent " + parentHash + '\n';
    }
    if (!author.empty())
    {
        commitData += "author " + author + '\n';
    }
    commitData += "message " + message + '\n';
    return commitData;
}

// generating commit hash from commit data
string createCommit(const string &treeHash, const string &parentHash, const string &author, const string &message)
{
    string commitData = createCommitData(treeHash, parentHash, author, message);
    string commitHash = sha256(commitData);
    storeObject(commitHash, commitData);
    return commitHash;
}

// getting the current brach from the HEAD file
string getCurrentBranch()
{
    fs::path headPath = ".mygit/HEAD";
    ifstream headFile(headPath);
    if (!headFile.is_open())
    {
        throw runtime_error("failed to open HEAD file\n");
    }
    string line;
    if (!getline(headFile, line))
    {
        throw runtime_error("Failed to read HEAD");
    }

    if (line.rfind("ref: ", 0) != 0)
    {
        throw runtime_error("Invalid HEAD format");
    }

    return line.substr(5);
}

// getting the current commit hash
string getCurrentCommit()
{
    string currentBranch = getCurrentBranch();
    fs::path branchPath = ".mygit/" + currentBranch;
    ifstream branchFile(branchPath);
    if (!branchFile)
    {
        return "";
    }
    string currentCommitHash;
    if (!getline(branchFile, currentCommitHash))
    {
        throw runtime_error("Failed to read commit hash");
    }
    branchFile.close();
    return currentCommitHash;
}

// updating branch ref
void updateBranchRef(const string &commitHash)
{
    string currentBranch = getCurrentBranch();
    fs::path currentBranchPath = ".mygit/" + currentBranch;
    fs::create_directories(currentBranchPath.parent_path());
    ofstream currentBranchFile(currentBranchPath);
    if (!currentBranchFile.is_open())
    {
        throw runtime_error("Failed to open current branch file\n");
    }
    currentBranchFile << commitHash;
    currentBranchFile.close();
}

// creating and saving a commit
string commitChanges(const string &message, const string &author)
{
    map<string, string> indexMap = loadIndex();
    string treeHash = createTree(indexMap);
    string parentHash = getCurrentCommit();
    string commitHash = createCommit(treeHash, parentHash, author, message);
    updateBranchRef(commitHash);
    return commitHash;
}

// common function for reading objects from hash
string readObject(const string &hash)
{
    if (hash.size() != 64)
    {
        throw runtime_error("Invalid object hash");
    }
    string dirName = hash.substr(0, 2);
    string fileName = hash.substr(2);
    fs::path objPath = ".mygit/objects/" + dirName + "/" + fileName;
    return readFile(objPath);
}

// getting parent commit hash from the commit data
string getParentFromCommit(const string &commitData)
{
    stringstream ss(commitData);
    string line;

    while (getline(ss, line))
    {
        if (line.rfind("parent ", 0) == 0)
        {
            return line.substr(7);
        }
    }

    return "";
}

// getting message from commit data
string getMessageFromCommit(const string &commitData)
{
    stringstream ss(commitData);
    string line;
    while (getline(ss, line))
    {
        if (line.rfind("message ", 0) == 0)
        {
            return line.substr(8);
        }
    }
    return "";
}

// getting author from commit data
string getAuthorFromCommit(const string &commitData)
{
    stringstream ss(commitData);
    string line;

    while (getline(ss, line))
    {
        if (line.rfind("author ", 0) == 0)
        {
            return line.substr(7);
        }
    }

    return "";
}

// logging commit
void logCommits()
{
    string currentCommitHash = getCurrentCommit();

    while (!currentCommitHash.empty())
    {
        string commitData = readObject(currentCommitHash);

        cout << "commit " + currentCommitHash + '\n';
        cout << "Author: "+getAuthorFromCommit(commitData) + "\n";
        cout << "Message: "+getMessageFromCommit(commitData) + "\n\n";

        currentCommitHash = getParentFromCommit(commitData);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "No commands provided\n";
        return 1;
    }
    string command = argv[1];
    if (command == "init")
    {
        initRepository();
    }
    else if (command == "config")
    {
        if (argc < 4 || string(argv[2]) != "user.name")
        {
            cout << "Usage: miniGit config user.name <name>\n";
            return 1;
        }
        setConfig(argv[3]);
    }
    else if (command == "add")
    {
        if (argc < 3)
        {
            cout << "Usage: miniGit add <file>\n";
            return 1;
        }
        addFile(argv[2]);
    }
    else if (command == "commit")
    {
        if (argc < 4 || string(argv[2]) != "-m")
        {
            cout << "Usage: miniGit commit -m <message> [--author <author>]\n";
            return 1;
        }

        string message = argv[3];
        string author;

        if (argc == 4)
        {
            author = getAuthor();

            if (author.empty())
            {
                cerr << "Author is not configured\n";
                cout << "Use 'miniGit config user.name <name>' or "
                     << "'--author <author>'\n";
                return 1;
            }
        }
        else if (argc == 6 && string(argv[4]) == "--author")
        {
            author = argv[5];
        }
        else
        {
            cout << "Usage: miniGit commit -m <message> [--author <author>]\n";
            return 1;
        }

        string commitHash = commitChanges(message, author);

        cout << "Committed: " << commitHash << '\n';
    }
    else if (command == "log")
    {
        logCommits();
    }
    else
    {
        cerr << "Unknown command: " << command << '\n';
        return 1;
    }
    return 0;
}