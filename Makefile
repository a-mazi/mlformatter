# Copyright © Artur Maziarek MMXX
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>

BINDIR   := bin
DEBUGDIR := debug
SRCDIR   := ./
INTERMEDIATESDIR := intermediates
OBJDIR_RELEASE   := $(BINDIR)/$(INTERMEDIATESDIR)
OBJDIR_DEBUG     := $(DEBUGDIR)/$(INTERMEDIATESDIR)


TARGET_APP   := mlformatter
SRCFILES_APP := $(wildcard $(SRCDIR)/*.cpp)
OBJFILES_APP_RELEASE := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR_RELEASE)/%.o,$(SRCFILES_APP))
OBJFILES_APP_DEBUG   := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR_DEBUG)/%.o,$(SRCFILES_APP))
DEPFILES += $(OBJFILES_APP_RELEASE:.o=.d)
DEPFILES += $(OBJFILES_APP_DEBUG:.o=.d)

CXXFLAGS := -Wall -c -fmessage-length=0 -std=c++17 -MMD -MP
RELEASE := \"$(shell echo `git log --date=short --pretty='%ad_%h' -1``git status -s` | sed s/\\s/_/g)\"


release: app_release
debug:   app_debug
clean:
	rm -rf $(BINDIR) $(DEBUGDIR)


app_release: CXXFLAGS += -DLOGLEVEL=3 -DRELEASE=$(RELEASE)
app_release: $(BINDIR)/$(TARGET_APP)
$(BINDIR)/$(TARGET_APP): $(OBJFILES_APP_RELEASE)
	g++ -o $@ $^

app_debug: CXXFLAGS += -DLOGLEVEL=4
app_debug: $(DEBUGDIR)/$(TARGET_APP)
$(DEBUGDIR)/$(TARGET_APP): $(OBJFILES_APP_DEBUG)
	g++ -o $@ $^


$(OBJDIR_RELEASE)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	g++ -O3 $(CXXFLAGS) -MF$(@:%.o=%.d) -MT$@ -o $@ $<

$(OBJDIR_DEBUG)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	g++ -O0 -g3 $(CXXFLAGS) -MF$(@:%.o=%.d) -MT$@ -o $@ $<

-include $(DEPFILES)
