#ifndef FILE_MANAGER_EMULATOR
#define FILE_MANAGER_EMULATOR

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>

using std::set;
using std::map;
using std::string;
using std::vector;
using std::endl;
using std::cout;
using std::cin;
using std::ifstream;
using std::stringstream;
using std::find;
using std::reverse;
using std::getline;

class FileManagerEmulator {
public:
	//constructs the object
	FileManagerEmulator() : root_(File("C:", File::directory)), curdir_("C:") {}
	//destructs the object
	virtual ~FileManagerEmulator() {DelDir(&root_);}

	//executes batch file, containing supported commands' execution order
	void ExecBatchFile(const string &inFile) {
		ifstream ifs(inFile.c_str());
		if(ifs.fail()) 
			ErrorMsg("Cannot read from " + inFile + " file, the program will terminate.");

		stringstream ss;
		for(string s, cmd, arg1, arg2; getline(ifs, s); ) {
			ss.clear();
			ss.str(s);
			ss >> cmd >> arg1 >> arg2;

			ToLower(cmd);
			if(cmd == "md") MakeDirectory(arg1);
			else if(cmd == "cd") ChangeCurDirectory(arg1);
			else if(cmd == "rd") RemoveDirectory(arg1);
			else if(cmd == "deltree") DeleteTree(arg1);
			else if(cmd == "mf") MakeFile(arg1);
			else if(cmd == "mhl") MakeHardLink(arg1, arg2);
			else if(cmd == "mdl") MakeDynamicLink(arg1, arg2);
			else if(cmd == "del") DeleteFile(arg1);
			else if(cmd == "copy") CopyFile(arg1, arg2);
			else if(cmd == "move") MoveFile(arg1, arg2);
		}
	}

	//creates a directory
	void MakeDirectory(const string &dname) {
		string parentdir, name, path__ = MakeAbsPath(dname);
		SplitPath(path__, parentdir, name);
		File *dir = Path2File(parentdir);
		if(!dir) 
			ErrorMsg("MD failed! " + dname + " has not a valid path, the program will terminate.");
		if(!IsValidFileName(name)) 
			ErrorMsg("MD failed! " + dname + " is not a valid name, the program will terminate.");
		if(!dir->HasDir(name) && !dir->HasFile(name)) 
			dir->subdirs_.insert(new File(name, File::directory, dir));
	}
	//changes the current directory
	void ChangeCurDirectory(const string &dname) {
		string parentdir, name, path__ = MakeAbsPath(dname);
		SplitPath(path__, parentdir, name);
		File *dir = parentdir.length() ? Path2File(parentdir) : &root_;
		if(!dir) 
			ErrorMsg("CD failed! " + dname + " has not a valid path, the program will terminate.");
		if(!IsValidFileName(name)) 
			ErrorMsg("CD failed! " + dname + " is not a valid name, the program will terminate.");
		curdir_ = path__;
	}
	//removes a directory if it is empty (doesn’t contain any files or subdirectories)
	void RemoveDirectory(const string &dname) {
		string parentdir, name, path__ = MakeAbsPath(dname);
		if(path__ == curdir_)
			ErrorMsg(string("RD failed! ") + "The current directory (" + curdir_ + ") cannot be deleted, the program will terminate.");
		File *dir = Path2File(path__);
		if(!dir) 
			ErrorMsg("RD failed! " + dname + " has not valid path, the program will terminate.");
		if(dir->HasHardLinks()) 
			ErrorMsg("RD failed! The directory " + name + " has hard link(s), the program will terminate.");
		if(dir->IsEmptyDir()) {
			SplitPath(path__, parentdir, name);
			dir->DelDynLinks();
			Path2File(parentdir)->subdirs_.erase(dir);
			delete dir;
		}
	}
	//removes a directory with all its subdirectories
	void DeleteTree(const string &path) {
		string parentdir, name, path__ = MakeAbsPath(path);
		SplitPath(path__, parentdir, name);
		File *pardir = Path2File(parentdir), *dir;
		if(!pardir) ErrorMsg("DELTREE failed! " + path + " is not valid, the program will terminate.");
		dir = pardir->HasDir(name);
		if(!dir) ErrorMsg("DELTREE failed! " + path + " is not valid, the program will terminate.");

		//can’t remove a directory which contains the current directory as one of its subdirectories.
		vector<string> dirs1, dirs2;
		TokenizeString(path__, dirs1);
		TokenizeString(curdir_, dirs2);
		if(dirs1.size() <= dirs2.size()) {
			bool contains = true;
			for(size_t i = 0, n = dirs1.size(); i < n; ++i)
				if(dirs1[i] != dirs2[i]) {contains = false; break;} 
			if(contains)
				ErrorMsg("DELTREE failed! " + path + " cannot be deleted since it contains the current directory, the program will terminate");
		}
		DelDir(dir, false);
	}
	//creates a file
	void MakeFile(const string &fname) {
		string parentdir, name, path__ = MakeAbsPath(fname);
		SplitPath(path__, parentdir, name);
		File *dir = Path2File(parentdir);
		if(!dir) 
			ErrorMsg("MF failed! " + path__ + " is not a valid path, the program will terminate.");
		if(!IsValidFileName(name)) 
			ErrorMsg("MF failed! " + fname + " is not a valid name, the program will terminate.");
		if(!dir->HasFile(name) && !dir->HasDir(name)) 
			dir->files_.insert(new File(name, File::file, dir));
	}
	//creates a hard link to a file/directory and places it in given location
	void MakeHardLink(const string &srcpath, const string &dstpath) {
		File *dst = Path2File(MakeAbsPath(dstpath)), *src, *sdir, *sfile, *lnk;
		if(!dst) ErrorMsg("MHL failed! " + dstpath + " is not a valid path, the program will terminate.");
		string parentdir, name, path__ = MakeAbsPath(srcpath);

		SplitPath(path__, parentdir, name);
		src = Path2File(parentdir);
		if(!src) ErrorMsg("MHL failed! " + parentdir + " is not a valid path, the program will terminate.");
		sdir = src->HasDir(name);
		sfile = src->HasFile(name);
		if(!sdir && !sfile)
			ErrorMsg("MHL failed! " + path__ + " is not a file or directory, the program will terminate.");
		if(dst->HasFile((name = "hlink[" + path__ + "]"))) return;
		lnk = new File(name, File::hardlink, dst);
		if(sdir) sdir->hlinks_[lnk] = dst;
		if(sfile) sfile->hlinks_[lnk] = dst;
		dst->files_.insert(lnk);
	}
	//creates a dynamic link to a file/directory and places it in given location
	void MakeDynamicLink(const string &srcpath, const string &dstpath) {
		File *dst = Path2File(MakeAbsPath(dstpath)), *src, *sdir, *sfile, *lnk;
		if(!dst) ErrorMsg("MDL failed! " + dstpath + " is not a valid path, the program will terminate.");
		string parentdir, name, path__ = MakeAbsPath(srcpath);

		SplitPath(path__, parentdir, name);
		src = Path2File(parentdir);
		if(!src) ErrorMsg("MDL failed! " + parentdir + " is not a valid path, the program will terminate.");
		sdir = src->HasDir(name);
		sfile = src->HasFile(name);
		if(!sdir && !sfile)
			ErrorMsg("MDL failed! " + path__ + " is not a file or directory, the program will terminate.");
		if(dst->HasFile((name = "dlink[" + path__ + "]"))) return;
		lnk = new File(name, File::dynlink, dst);
		if(sdir) sdir->dlinks_[lnk] = dst;
		if(sfile) sfile->dlinks_[lnk] = dst;
		dst->files_.insert(lnk);
	}
	//removes a file or link
	void DeleteFile(const string &fname) {
		File *dir, *file;
		string parentdir, name, path__ = MakeAbsPath(fname);
		
		SplitPath(path__, parentdir, name);
		dir = Path2File(parentdir);
		if(!dir) ErrorMsg("DEL failed! " + parentdir + " is not a valid path, the program will terminate.");
		file = dir->HasFile(name);
		if(!file) ErrorMsg("DEL failed! There is no " + name + " file, the program will terminate.");
		if(file->HasHardLinks()) 
			ErrorMsg("DEL failed! The file " + name + " has hard link(s), the program will terminate.");
		file->DelDynLinks();
		dir->files_.erase(file);
		delete file;
	}
	//copies an existed directory/file/link to another location
	void CopyFile(const string &srcpath, const string &dstpath) {
		if(srcpath == dstpath || srcpath.length() < 3) return;
		File *dstpdir, *dstdir, *srcpdir, *srcdir;
		bool notrootdir;
		string parentdir, name, path__ = MakeAbsPath(dstpath);
		SplitPath(path__, parentdir, name);
		dstpdir = Path2File((notrootdir = parentdir.length() > 0) ? parentdir : name);
		if(!dstpdir) ErrorMsg("COPY failed! " + parentdir + " is not a valid path, the program will terminate.");
		dstdir = dstpdir->HasDir(name);
		if(!dstdir && notrootdir) ErrorMsg("COPY failed! There is no " + name + " directory, the program will terminate.");
		if(!notrootdir) dstdir = dstpdir;

		path__ = MakeAbsPath(srcpath);
		SplitPath(path__, parentdir, name);
		srcpdir = Path2File(parentdir);
		if(!srcpdir) ErrorMsg("DEL failed! " + parentdir + " is not a valid path, the program will terminate.");
		srcdir = srcpdir->HasDir(name);
		if(!srcdir) srcdir = srcpdir->HasFile(name);
		if(!srcdir) ErrorMsg("DEL failed! There is no " + name + " file or directory, the program will terminate.");

		if(srcdir->IsDirectory()) dstdir->subdirs_.insert(CopyTree(srcdir, dstdir)); 
		else dstdir->files_.insert(new File(srcdir->name_, srcdir->type_, srcdir->parent_));
	}
	//moves an existing directory/file/link to another location
	void MoveFile(const string &srcpath, const string &dstpath) {
		CopyFile(srcpath, dstpath);
		string parentdir, name, path__ = MakeAbsPath(srcpath);
		SplitPath(path__, parentdir, name);
		File *dir = Path2File(parentdir);
		if(!dir) ErrorMsg("MOVE failed! " + parentdir + " is invalid path, the program will terminate.");
		if(dir->HasDir(name)) DeleteTree(srcpath);
		else if(dir->HasFile(name)) DeleteFile(srcpath);
		else ErrorMsg("MOVE failed! " + srcpath + " doesn't exist, the program will terminate");
		UpdateDynLinks(MakeAbsPath(dstpath) + "\\" + name);
	}

	//prints the directory structure
	inline void Print() const {root_.Print();}
	//shows an error message (and quits)
	static void ErrorMsg(const string &msg, bool stop = true) {cout << msg << endl; if(stop) {system("pause"); exit(1);}}

protected:
	struct File;
	struct FileCmp {
		bool operator()(const File *lhs,  const File *rhs) const {
			return strcmp(lhs->name_.c_str(), rhs->name_.c_str()) < 0;
		}
	};
	struct File {
		enum Type {file, directory, hardlink, dynlink};
		typedef set<File *, FileCmp> FileSet;
		typedef map<File *, File *, FileCmp> LinkMap;
		FileSet files_, subdirs_;
		//key of LinkMap holds the link, value - parent directory
		LinkMap hlinks_, dlinks_;
		string name_;
		Type type_;
		File *parent_;
		
		//construct a file
		File(const string &name, Type t = file, File *p = 0) : name_(name), type_(t), parent_(p) {}

		//deletes all its files if the object is directory
		void DeleteFiles(bool ignore_hard_links = true) {
			FileSet::const_iterator start = files_.begin(), end = files_.end(), i;
			for(i = start; i != end; ++i) 
				if(!ignore_hard_links && (*i)->HasHardLinks())
					ErrorMsg("Cannot delete " + (*i)->name_ + " file, it has hard link(s), the program will terminate.");
				else delete *i;
			files_.clear();
		}
		//deletes all dynamic links which might have this file/dir/link
		void DelDynLinks() {
			for(LinkMap::iterator i = dlinks_.begin(), last = dlinks_.end(); i != last; ++i) {
				i->second->files_.erase(i->first);
				delete i->first;
			}
			dlinks_.clear();
		}
		//prints directory structure
		void Print(string padding = "") const {
			cout << name_ << endl;
			padding += "  ";
			FileSet::const_iterator i, last;
			for(i = files_.begin(), last = files_.end(); i!=last; ++i)
				cout << padding << (*i)->name_ << endl;
			for(i = subdirs_.begin(), last = subdirs_.end(); i!=last; ++i) {
				cout << padding; 
				(*i)->Print(padding);
			}
		}
		//checks if this directory has files
		File* HasFile(const string &fname) {
			FileSet::const_iterator start = files_.begin(), end = files_.end(), i;
			for(i = start; i != end; ++i) if((*i)->name_ == fname) return *i;
			return 0;
		}
		//checks if this directory has subdirectories
		File* HasDir(const string &dname) {
			FileSet::const_iterator start = subdirs_.begin(), end = subdirs_.end(), i;
			for(i = start; i != end; ++i) if((*i)->name_ == dname) return *i;
			return 0;
		}
		inline bool HasDynLinks() const {return !dlinks_.empty();}
		inline bool HasHardLinks() const {return !hlinks_.empty();}
		inline bool IsFile() const {return type_ == file;}
		inline bool IsDirectory() const {return type_ == directory;}
		inline bool IsHardLink() const {return type_ == hardlink;}
		inline bool IsDynLink() const {return type_ == dynlink;}
		inline bool IsEmptyDir() const {return IsDirectory() && files_.empty() && subdirs_.empty() && hlinks_.empty() && dlinks_.empty();}
	};

	//tokenizes the input string according to given delimiters and fills tokens into vector
	static void TokenizeString(const string &s, vector<string> &strs, const string &delims = "\\") {
		char *context = 0;
		strs.clear();
		char *sdup = _strdup(s.c_str()), *ps = strtok_s(sdup, delims.c_str(), &context);
		while(ps) {strs.push_back(ps);  ps = strtok_s(0, delims.c_str(), &context);}
		free(sdup);
	}
	//makes an absolute path 
	string MakeAbsPath(const string &path) {
		return ((path[0] == 'C' || path[0] == 'c') && path[1] == ':') ? path : curdir_ + string("\\") + path;
	}
	//validates a file name
	static bool IsValidFileName(const string &fname) {
		if(!fname.length()) return false;
		if(fname == "C:" || fname == "c:") return true;

		string ext, name;
		bool afterpnt = false;
		for(string::const_reverse_iterator i = fname.rbegin(), last = fname.rend(); i != last; ++i) {
			if(!afterpnt && *i == '.') {afterpnt = true; continue;}
			if((*i < 'a' || *i > 'z') &&
			   (*i < 'A' || *i > 'Z') &&
			   (*i < '0' || *i > '9') && 
			    *i != '.') return false;
			afterpnt ? name += *i : ext += *i;
		}
		reverse(name.begin(), name.end());
		reverse(ext.begin(), ext.end());
		if(name == "") {name = ext; ext = "";}
		return name.length() <= 8 && ext.length() <= 3;
	}
	//validates a path and returns corresponding object's pointer
	File* Path2File(const string &path) {
		if(!path.length()) return 0;

		bool found;
		File *file = &root_;
		vector<string> dirs;
		string path__ = MakeAbsPath(path);

		File::FileSet::const_iterator start = root_.subdirs_.begin(), end = root_.subdirs_.end(), ii;
		TokenizeString(path__, dirs);
		
		for(size_t i = 1, n = dirs.size(); i < n; ++i) {
			found = false;
			for(ii = start; ii != end; ++ii)
				if((*ii)->name_ == dirs[i]) {
					found = true; 
					start = (*ii)->subdirs_.begin(); 
					end = (*ii)->subdirs_.end();
					file = *ii;
					break;
				}
			if(!found) return 0;
		}
		return file;
	}
	//splits the input full path into parent directory's path and file/directory name
	void SplitPath(const string &path, string &ppath, string &fname) {
		ppath = fname = "";
		bool afterslash = false, betweenbraces = false;
		for(string::const_reverse_iterator i = path.rbegin(), last = path.rend(); i!=last; ++i) {
			if(*i == ']') betweenbraces = true;
			if(*i == '[') betweenbraces = false;
			if(!afterslash && !betweenbraces && *i == '\\') {afterslash = true; continue;}
			afterslash ? ppath += *i : fname += *i;
		}
		reverse(ppath.begin(), ppath.end());
		reverse(fname.begin(), fname.end());
	}
	//duplicates node's structure and inserts into destination node, returns the newly duplicated node
	File* CopyTree(File *node, File *dst) {
		File* new_node = new File(node->name_, node->type_, dst), *p;
		new_node->dlinks_ = node->dlinks_;
		new_node->hlinks_ = node->hlinks_;

		File::FileSet::const_iterator start = node->files_.begin(), end = node->files_.end(), i;
		for(i = start; i != end; ++i) {
			p = new File((*i)->name_, (*i)->type_, new_node);
			p->dlinks_ = (*i)->dlinks_;
			p->hlinks_ = (*i)->hlinks_;
			new_node->files_.insert(p);
		}
		start = node->subdirs_.begin();
		end = node->subdirs_.end();
		for(i = start; i != end; ++i) new_node->subdirs_.insert(CopyTree(*i, new_node));
		return new_node;
	}
	//updates the dynamic links to the files/directories present in source
	//generally needed after move command
	void UpdateDynLinks(const string &path) {
		string ppath, name;
		File *dir, *file;
		SplitPath(path, ppath, name);
		dir = Path2File(ppath);
		file = dir->HasDir(name);
		if(!file) file = dir->HasFile(name);
		if(!file) return;

		//update current file/directory
		File::LinkMap::const_iterator i = file->dlinks_.begin(), last = file->dlinks_.end();
		for(; i != last; ++i)
			i->first->name_ = "dlink[" + path + "]";

		//update all files if 'file' is directory
		File::FileSet::const_iterator ii = file->files_.begin(), llast = file->files_.end();
		for(; ii != llast; ++ii)
			for(i = (*ii)->dlinks_.begin(), last = (*ii)->dlinks_.end(); i != last; ++i)
				i->first->name_ = "dlink[" + path + "\\" + (*ii)->name_+ "]";

		//update all subdirectories if file is directory
		ii = file->subdirs_.begin();
		llast = file->subdirs_.end();
		for(; ii != llast; ++ii) UpdateDynLinks(path + "\\" + (*ii)->name_);
	}
	void DelDir(File *dir, bool ignore_hard_links = true) {
		if(!dir) return;
		if(!ignore_hard_links && dir->HasHardLinks())
			ErrorMsg("Cannot delete " + dir->name_ + ", it has hard links, the program will terminate");

		//delete the files
		dir->DeleteFiles(ignore_hard_links);
		//delete the subdirectories
		while(!(dir->subdirs_.empty())) DelDir(*(dir->subdirs_.begin()), ignore_hard_links);
		dir->subdirs_.clear();
		//notify the parent directory about deletion and delete itself
		if(dir != &root_) {
			dir->parent_->subdirs_.erase(dir);
			delete dir;
		}
	}
	static void ToLower(string &s) {for(size_t i = 0, n = s.size(); i < n; s[i] = tolower(s[i]), ++i);}
	File root_; 
	string curdir_;
};
#endif