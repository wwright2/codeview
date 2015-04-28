

**Adding an existing project to GitHub using the command line**

    mac
    windows
    linux
    all

Putting your existing work on GitHub can let you share and collaborate in lots of great ways.
For more information, see "Remove sensitive data."

Create New Repository drop-down Create a new repository on GitHub. To avoid errors, do not initialize the new repository with README, license, or gitignore files. You can add these files after your project has been pushed to GitHub.

Open Terminal (for Mac users) or the command prompt (for Windows and Linux users).Change the current working directory to your local project.  

- Initialize the local directory as a Git repository.  
- Add the files in your new local repository. This stages them for the first commit.  

```
git init
git add .
```
Adds the files in the local repository and stages them for commit. To unstage a file, use 'git reset HEAD YOUR-FILE'.

Commit the files that you've staged in your local repository.
```
  git commit -m 'First commit'
```
Commits the tracked changes and prepares them to be pushed to a remote repository. To remove this commit and modify the file, use 'git reset --soft HEAD~1' and commit and add the file again.

Copy remote repository URL fieldAt the top of your GitHub repository's Quick Setup page, click to copy the remote repository URL.

In the Command prompt, add the URL for the remote repository where your local repository will be pushed.

```
git remote add origin <remote repository URL>
```

**Sets the new remote**  
```
git remote -v
```
**Verifies the new remote URL**

Note: GitHub for Windows users should use the command git remote set-url origin instead of git remote add origin here.
Push the changes in your local repository to GitHub.
```
git push origin master
``` 
Pushes the changes in your local repository up to the remote repository you specified as the origin



**Example:**

Quick setup — if you've done this kind of thing before
or
HTTPS SSH

We recommend every repository include a README, LICENSE, and .gitignore.
…or create a new repository on the command line  

```
echo # tisdk.daisy.1.6 >> README.md
git init
git add README.md
git commit -m "first commit"

git add .
git status
git commit -m " commit recently added files "
git remote add github https://github.com/wwright2/tisdk.daisy.1.6.git
git commit -m 'start a clone of tisdk that I can update'
git remote -v
git push github master

```
**Since:** 'origin' existed as a ti github project changed name to github for my gh repo.


…or push an existing repository from the command line  
```
git remote add origin https://github.com/wwright2/tisdk.daisy.1.6.git
git push -u origin master
```

…or import code from another repository

You can initialize this repository with code from a Subversion, Mercurial, or TFS project.


###Push Remote1, Pull Remote2
```
git config remote.origin.pushurl user@user.com:repo.git
```
Configure a Seperate push url.   Now, git pull, will pull from the original clone URL but git push will push to the other.


###Git Remote
```
usage: git remote [-v | --verbose]
   or: git remote add [-t <branch>] [-m <master>] [-f] [--tags|--no-tags] [--mirror=<fetch|push>] <name> <url>
   or: git remote rename <old> <new>
   or: git remote remove <name>
   or: git remote set-head <name> (-a | --auto | -d | --delete |<branch>)
   or: git remote [-v | --verbose] show [-n] <name>
   or: git remote prune [-n | --dry-run] <name>
   or: git remote [-v | --verbose] update [-p | --prune] [(<group> | <remote>)...]
   or: git remote set-branches [--add] <name> <branch>...
   or: git remote set-url [--push] <name> <newurl> [<oldurl>]
   or: git remote set-url --add <name> <newurl>
   or: git remote set-url --delete <name> <url>

    -v, --verbose         be verbose; must be placed before a subcommand
```

```

### Config 
usage: git config [options]

Config file location
    --global              use global config file
    --system              use system config file
    --local               use repository config file
    -f, --file <file>     use given config file
    --blob <blob-id>      read config from given blob object

Action
    --get                 get value: name [value-regex]
    --get-all             get all values: key [value-regex]
    --get-regexp          get values for regexp: name-regex [value-regex]
    --get-urlmatch        get value specific for the URL: section[.var] URL
    --replace-all         replace all matching variables: name value [value_regex]
    --add                 add a new variable: name value
    --unset               remove a variable: name [value-regex]
    --unset-all           remove all matches: name [value-regex]
    --rename-section      rename section: old-name new-name
    --remove-section      remove a section: name
    -l, --list            list all
    -e, --edit            open an editor
    --get-color <slot>    find the color configured: [default]
    --get-colorbool <slot>
                          find the color setting: [stdout-is-tty]

Type
    --bool                value is "true" or "false"
    --int                 value is decimal number
    --bool-or-int         value is --bool or --int
    --path                value is a path (file or directory name)

Other
    -z, --null            terminate values with NUL byte
    --includes            respect include directives on lookup
```

