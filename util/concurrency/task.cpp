// @file task.cpp

/**
*    Copyright (C) 2008 10gen Inc.
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,b
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "pch.h"
#include "boost/any.hpp"
#include "task.h"
#include "../goodies.h"
#include "../unittest.h"
#include "boost/thread/condition.hpp"

namespace mongo { 

    namespace task { 

        /*void foo() { 
            boost::mutex m;
            boost::mutex::scoped_lock lk(m);
            boost::condition cond;
            cond.wait(lk);
            cond.notify_one();
        }*/

        Task::Task() { 
            {
                
                any a;
                a = 3;
                a = string("AAA");
                string x = any_cast<string>(a);
                if( a.type() == typeid(int) )
                    cout << "won't print" << endl;
            }

            n = 0;
            repeat = 0;
        }

        void Task::halt() { repeat = 0; }

        void Task::run() { 
            assert( n == 0 );
            while( 1 ) {
                n++;
                try { 
                    doWork();
                } 
                catch(...) { }
                if( repeat == 0 )
                    break;
                sleepmillis(repeat);
            }
        }

        void Task::ending() { me.reset(); }

        void Task::begin(shared_ptr<Task> t) {
            me = t;
            go();
        }

        void fork(shared_ptr<Task> t) { 
            t->begin(t);
        }

        void repeat(shared_ptr<Task> t, unsigned millis) { 
            t->repeat = millis;
            t->begin(t);
        }

    }
}

#include "msg.h"

/* class Task::Port */

namespace mongo {
    namespace task {

        void Port::send(const any& msg) { 
            {
                boost::mutex::scoped_lock lk(m);
                d.push_back(msg);
            }
            c.notify_one();
        }

        void Port::doWork() { 
            while( 1 ) { 
                any a;
                {
                    boost::mutex::scoped_lock lk(m);
                    while( d.empty() )
                        c.wait(m);
                    a = d.front();
                    d.pop_front();
                }
                try {
                    if( !got(a) )
                        break;
                } catch(std::exception& e) { 
                    log() << "Port::doWork() exception " << e.what() << endl;
                }
            }
        }
        
        class PortTest : public Port {
        protected:
            void got(const int& msg) { }
        public:
            virtual bool got(const any& msg) { 
                assert( any_cast<int>(msg) <= 55 );
                return any_cast<int>(msg) != 55;
            }
            virtual string name() { return "PortTest"; }
        };    

        struct PortUnitTest : public UnitTest {
            void run() { 
                PortTest *p = new PortTest();
                shared_ptr<Task> tp = p->taskPtr();
                fork( tp );
                p->send(3);
                p->send(55);
            } 
        } portunittest;
    }
}