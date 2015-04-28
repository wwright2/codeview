

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
git remote add origin https://github.com/wwright2/tisdk.daisy.1.6.git
git push -u origin master
```
…or push an existing repository from the command line  
```
git remote add origin https://github.com/wwright2/tisdk.daisy.1.6.git
git push -u origin master
```

…or import code from another repository

You can initialize this repository with code from a Subversion, Mercurial, or TFS project.

