/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef SUBJECT_IMP_PRIVATE_H
#define SUBJECT_IMP_PRIVATE_H

#include "TypesFile.h"

#include <boost/any.hpp>
#include <list>
#include <map>
#include <string>
#include <vector>

class SafeSlot;
class Slot;
class Subject;

class SubjectImpPrivate
{
   typedef std::map<std::string,std::list<SafeSlot> > MapType;

public:
   SubjectImpPrivate();
   virtual ~SubjectImpPrivate();
   virtual bool attach(Subject &subject, const std::string &signal, const Slot &slot);
   virtual bool detach(Subject &subject, const std::string &signal, const Slot &slot);
   void notify(Subject &subject, const std::string &signal, const std::string &originalSignal, const boost::any &data=boost::any());
   const std::list<SafeSlot>& getSlots(const std::string & signal);
   void removeEmptySlots(const std::string &recursion, std::list<SafeSlot> &slotVec);

private:
   MapType mSlots;
   std::vector<std::string> mRecursions;
   Subject *mpSubject;
};

#endif