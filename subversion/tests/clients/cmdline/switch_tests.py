#!/usr/bin/env python
#
#  switch_tests.py:  testing `svn switch'.
#
#  Subversion is a tool for revision control. 
#  See http://subversion.tigris.org for more information.
#    
# ====================================================================
# Copyright (c) 2000-2003 CollabNet.  All rights reserved.
#
# This software is licensed as described in the file COPYING, which
# you should have received as part of this distribution.  The terms
# are also available at http://subversion.tigris.org/license-1.html.
# If newer versions of this license are posted there, you may use a
# newer version instead, at your option.
#
######################################################################

# General modules
import shutil, string, sys, re, os

# Our testing module
import svntest

# (abbreviation)
Skip = svntest.testcase.Skip
XFail = svntest.testcase.XFail
Item = svntest.wc.StateItem


### Bummer.  It would be really nice to have easy access to the URL
### member of our entries files so that switches could be testing by
### examining the modified ancestry.  But status doesn't show this
### information.  Hopefully in the future the cmdline binary will have
### a subcommand for dumping multi-line detailed information about
### versioned things.  Until then, we'll stick with the traditional
### verification methods.
###
### gjs says: we have 'svn info' now

def get_routine_status_state(wc_dir):
  """get the routine status list for WC_DIR at the completion of an
  initial call to do_routine_switching()"""
  
  # Construct some paths for convenience
  ADH_path = os.path.join(wc_dir, 'A', 'D', 'H')
  chi_path = os.path.join(ADH_path, 'chi')
  omega_path = os.path.join(ADH_path, 'omega')
  psi_path = os.path.join(ADH_path, 'psi')
  pi_path = os.path.join(ADH_path, 'pi')
  tau_path = os.path.join(ADH_path, 'tau')
  rho_path = os.path.join(ADH_path, 'rho')

  # Now generate a state
  state = svntest.actions.get_virginal_state(wc_dir, 1)
  state.remove('A/B/E', 'A/B/E/alpha', 'A/B/E/beta', 'A/B/F', 'A/B/lambda')
  state.add({
    'A/B/pi' : Item(status='  ', wc_rev=1, repos_rev=1),
    'A/B/tau' : Item(status='  ', wc_rev=1, repos_rev=1),
    'A/B/rho' : Item(status='  ', wc_rev=1, repos_rev=1),
    })

  return state

#----------------------------------------------------------------------

def get_routine_disk_state(wc_dir):
  """get the routine disk list for WC_DIR at the completion of an
  initial call to do_routine_switching()"""

  disk = svntest.main.greek_state.copy()

  # iota has the same contents as gamma
  disk.tweak('iota', contents=disk.desc['A/D/gamma'].contents)

  # A/B/* no longer exist, but have been replaced by copies of A/D/G/*
  disk.remove('A/B/E', 'A/B/E/alpha', 'A/B/E/beta', 'A/B/F', 'A/B/lambda')
  disk.add({
    'A/B/pi' : Item("This is the file 'pi'."),
    'A/B/rho' : Item("This is the file 'rho'."),
    'A/B/tau' : Item("This is the file 'tau'."),
    })

  return disk

#----------------------------------------------------------------------

def do_routine_switching(wc_dir, verify):
  """perform some routine switching of the working copy WC_DIR for
  other tests to use.  If VERIFY, then do a full verification of the
  switching, else don't bother."""

  ### Switch the file `iota' to `A/D/gamma'.

  # Construct some paths for convenience
  iota_path = os.path.join(wc_dir, 'iota')
  gamma_url = svntest.main.current_repo_url + '/A/D/gamma'

  if verify:
    # Create expected output tree
    expected_output = svntest.wc.State(wc_dir, {
      'iota' : Item(status='U '),
      })

    # Create expected disk tree (iota will have gamma's contents)
    expected_disk = svntest.main.greek_state.copy()
    expected_disk.tweak('iota',
                        contents=expected_disk.desc['A/D/gamma'].contents)

    # Create expected status tree
    expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
    expected_status.tweak('iota', switched='S')
  
    # Do the switch and check the results in three ways.
    if svntest.actions.run_and_verify_switch(wc_dir, iota_path, gamma_url,
                                             expected_output,
                                             expected_disk,
                                             expected_status):
      return 1
  else:
    svntest.main.run_svn(None, 'switch',
                         '--username', svntest.main.wc_author,
                         '--password', svntest.main.wc_passwd,
                         gamma_url, iota_path)
  
  ### Switch the directory `A/B' to `A/D/G'.

  # Construct some paths for convenience
  AB_path = os.path.join(wc_dir, 'A', 'B')
  ADG_url = svntest.main.current_repo_url + '/A/D/G'

  if verify:
    # Create expected output tree
    expected_output = svntest.wc.State(wc_dir, {
      'A/B/E'       : Item(status='D '),
      'A/B/F'       : Item(status='D '),
      'A/B/lambda'  : Item(status='D '),
      'A/B/pi' : Item(status='A '),
      'A/B/tau' : Item(status='A '),
      'A/B/rho' : Item(status='A '),
      })
    
    # Create expected disk tree (iota will have gamma's contents,
    # A/B/* will look like A/D/G/*)
    expected_disk = get_routine_disk_state(wc_dir)
    
    # Create expected status
    expected_status = get_routine_status_state(wc_dir)
    expected_status.tweak('iota', 'A/B', switched='S')
  
    # Do the switch and check the results in three ways.
    if svntest.actions.run_and_verify_switch(wc_dir, AB_path, ADG_url,
                                             expected_output,
                                             expected_disk,
                                             expected_status):
      return 1
  else:
    svntest.main.run_svn(None, 'switch', ADG_url, AB_path)

  return 0

#----------------------------------------------------------------------

def commit_routine_switching(wc_dir, verify):
  "Commit some stuff in a routinely-switched working copy."
  
  # Make some local mods
  iota_path = os.path.join(wc_dir, 'iota')
  Bpi_path = os.path.join(wc_dir, 'A', 'B', 'pi')
  Gpi_path = os.path.join(wc_dir, 'A', 'D', 'G', 'pi')
  Z_path = os.path.join(wc_dir, 'A', 'D', 'G', 'Z')
  zeta_path = os.path.join(wc_dir, 'A', 'D', 'G', 'Z', 'zeta')

  svntest.main.file_append(iota_path, "apple")
  svntest.main.file_append(Bpi_path, "watermelon")
  svntest.main.file_append(Gpi_path, "banana")
  os.mkdir(Z_path)
  svntest.main.file_append(zeta_path, "This is the file 'zeta'.")
  svntest.main.run_svn(None, 'add', Z_path)

  # Try to commit.  We expect this to fail because, if all the
  # switching went as expected, A/B/pi and A/D/G/pi point to the
  # same URL.  We don't allow this.
  if svntest.actions.run_and_verify_commit (wc_dir, None, None,
                                            "commit to a URL more than once",
                                            None, None, None, None,
                                            wc_dir):
    return 1

  # Okay, that all taken care of, let's revert the A/D/G/pi path and
  # move along.  Afterward, we should be okay to commit.  (Sorry,
  # holsta, that banana has to go...)
  svntest.main.run_svn(None, 'revert', Gpi_path)

  # Create expected output tree.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/Z' : Item(verb='Adding'),
    'A/D/G/Z/zeta' : Item(verb='Adding'),
    'iota' : Item(verb='Sending'),
    'A/B/pi' : Item(verb='Sending'),
    })

  # Created expected status tree.
  expected_status = get_routine_status_state(wc_dir)
  expected_status.tweak(repos_rev=2)
  expected_status.tweak('iota', 'A/B', switched='S')
  expected_status.tweak('iota', 'A/B/pi', wc_rev=2, status='  ')
  expected_status.add({
    'A/D/G/Z' : Item(status='  ', wc_rev=2, repos_rev=2),
    'A/D/G/Z/zeta' : Item(status='  ', wc_rev=2, repos_rev=2),
    })

  # Commit should succeed
  if verify:
    if svntest.actions.run_and_verify_commit(wc_dir,
                                             expected_output,
                                             expected_status,
                                             None, None, None, None, None,
                                             wc_dir):
      return 1
  else:
    svntest.main.run_svn(None, 'ci', '-m', 'log msg', wc_dir)

  return 0

######################################################################
# Tests
#
#   Each test must return 0 on success or non-zero on failure.

#----------------------------------------------------------------------

def routine_switching(sbox):
  "test some basic switching operations"
    
  if sbox.build():
    return 1

  # Setup (and verify) some switched things
  return do_routine_switching(sbox.wc_dir, 1)

#----------------------------------------------------------------------

def commit_switched_things(sbox):
  "commits after some basic switching operations"

  if sbox.build():
    return 1

  wc_dir = sbox.wc_dir
  
  # Setup some switched things (don't bother verifying)
  if do_routine_switching(wc_dir, 0):
    return 1

  # Commit some stuff (and verify)
  if commit_routine_switching(wc_dir, 1):
    return 1

  return 0
    
#----------------------------------------------------------------------

def full_update(sbox):
  "update wc that contains switched things"

  if sbox.build():
    return 1

  wc_dir = sbox.wc_dir
  
  # Setup some switched things (don't bother verifying)
  if do_routine_switching(wc_dir, 0):
    return 1

  # Copy wc_dir to a backup location
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.duplicate_dir(wc_dir, wc_backup)
  
  # Commit some stuff (don't bother verifying)
  if commit_routine_switching(wc_backup, 0):
    return 1

  # Some convenient path variables
  iota_path = os.path.join(wc_dir, 'iota')
  gamma_path = os.path.join(wc_dir, 'A', 'D', 'gamma')
  Bpi_path = os.path.join(wc_dir, 'A', 'B', 'pi')
  BZ_path = os.path.join(wc_dir, 'A', 'B', 'Z')
  Bzeta_path = os.path.join(wc_dir, 'A', 'B', 'Z', 'zeta')
  Gpi_path = os.path.join(wc_dir, 'A', 'D', 'G', 'pi')
  GZ_path = os.path.join(wc_dir, 'A', 'D', 'G', 'Z')
  Gzeta_path = os.path.join(wc_dir, 'A', 'D', 'G', 'Z', 'zeta')

  # Create expected output tree for an update of wc_backup.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='U '),
    'A/D/gamma' : Item(status='U '),
    'A/B/pi' : Item(status='U '),
    'A/B/Z' : Item(status='A '),
    'A/B/Z/zeta' : Item(status='A '),
    'A/D/G/pi' : Item(status='U '),
    'A/D/G/Z' : Item(status='A '),
    'A/D/G/Z/zeta' : Item(status='A '),
    })

  # Create expected disk tree for the update
  expected_disk = get_routine_disk_state(wc_dir)
  expected_disk.tweak('iota', contents="This is the file 'gamma'.apple")
  expected_disk.tweak('A/D/gamma', contents="This is the file 'gamma'.apple")
  expected_disk.tweak('A/B/pi', contents="This is the file 'pi'.watermelon")
  expected_disk.tweak('A/D/G/pi', contents="This is the file 'pi'.watermelon")
  expected_disk.add({
    'A/B/Z' : Item(),
    'A/B/Z/zeta' : Item(contents="This is the file 'zeta'."),
    'A/D/G/Z' : Item(),
    'A/D/G/Z/zeta' : Item(contents="This is the file 'zeta'."),
    })

  # Create expected status tree for the update.
  expected_status = get_routine_status_state(wc_dir)
  expected_status.tweak(repos_rev=2, wc_rev=2)
  expected_status.add({
    'A/D/G/Z' : Item(status='  ', wc_rev=2, repos_rev=2),
    'A/D/G/Z/zeta' : Item(status='  ', wc_rev=2, repos_rev=2),
    'A/B/Z' : Item(status='  ', wc_rev=2, repos_rev=2),
    'A/B/Z/zeta' : Item(status='  ', wc_rev=2, repos_rev=2),
    })
  expected_status.tweak('iota', 'A/B', switched='S')

  return svntest.actions.run_and_verify_update(wc_dir,
                                               expected_output,
                                               expected_disk,
                                               expected_status)

#----------------------------------------------------------------------

def full_rev_update(sbox):
  "reverse update wc that contains switched things"

  if sbox.build():
    return 1

  wc_dir = sbox.wc_dir
  
  # Setup some switched things (don't bother verifying)
  if do_routine_switching(wc_dir, 0):
    return 1

  # Commit some stuff (don't bother verifying)
  if commit_routine_switching(wc_dir, 0):
    return 1

  # Update to HEAD (tested elsewhere)
  svntest.main.run_svn (None, 'up', wc_dir)

  # Some convenient path variables
  iota_path = os.path.join(wc_dir, 'iota')
  gamma_path = os.path.join(wc_dir, 'A', 'D', 'gamma')
  Bpi_path = os.path.join(wc_dir, 'A', 'B', 'pi')
  BZ_path = os.path.join(wc_dir, 'A', 'B', 'Z')
  Gpi_path = os.path.join(wc_dir, 'A', 'D', 'G', 'pi')
  GZ_path = os.path.join(wc_dir, 'A', 'D', 'G', 'Z')

  # Now, reverse update, back to the pre-commit state.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='U '),
    'A/D/gamma' : Item(status='U '),
    'A/B/pi' : Item(status='U '),
    'A/B/Z' : Item(status='D '),
    'A/D/G/pi' : Item(status='U '),
    'A/D/G/Z' : Item(status='D '),
    })

  # Create expected disk tree
  expected_disk = get_routine_disk_state(wc_dir)
    
  # Create expected status
  expected_status = get_routine_status_state(wc_dir)
  expected_status.tweak(repos_rev=2)
  expected_status.tweak('iota', 'A/B', switched='S')

  return svntest.actions.run_and_verify_update(wc_dir,
                                               expected_output,
                                               expected_disk,
                                               expected_status,
                                               None, None, None,
                                               None, None, 1,
                                               '-r', '1', wc_dir)

#----------------------------------------------------------------------

def update_switched_things(sbox):
  "update switched wc things to HEAD"

  if sbox.build():
    return 1

  wc_dir = sbox.wc_dir
  
  # Setup some switched things (don't bother verifying)
  if do_routine_switching(wc_dir, 0):
    return 1

  # Copy wc_dir to a backup location
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.duplicate_dir(wc_dir, wc_backup)
  
  # Commit some stuff (don't bother verifying)
  if commit_routine_switching(wc_backup, 0):
    return 1

  # Some convenient path variables
  iota_path = os.path.join(wc_dir, 'iota')
  B_path = os.path.join(wc_dir, 'A', 'B')

  # Create expected output tree for an update of wc_backup.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='U '),
    'A/B/pi' : Item(status='U '),
    'A/B/Z' : Item(status='A '),
    'A/B/Z/zeta' : Item(status='A '),
    })

  # Create expected disk tree for the update
  expected_disk = get_routine_disk_state(wc_dir)
  expected_disk.tweak('iota', contents="This is the file 'gamma'.apple")

  expected_disk.tweak('A/B/pi', contents="This is the file 'pi'.watermelon")
  expected_disk.add({
    'A/B/Z' : Item(),
    'A/B/Z/zeta' : Item("This is the file 'zeta'."),
    })

  # Create expected status tree for the update.
  expected_status = get_routine_status_state(wc_dir)
  expected_status.tweak(repos_rev=2)
  expected_status.tweak('iota', 'A/B', switched='S')
  expected_status.tweak('A/B', 'A/B/pi', 'A/B/rho', 'A/B/tau', 'iota',
                        wc_rev=2)
  expected_status.add({
    'A/B/Z' : Item(status='  ', wc_rev=2, repos_rev=2),
    'A/B/Z/zeta' : Item(status='  ', wc_rev=2, repos_rev=2),
    })

  return svntest.actions.run_and_verify_update(wc_dir,
                                               expected_output,
                                               expected_disk,
                                               expected_status,
                                               None, None, None,
                                               None, None, 0,
                                               B_path,
                                               iota_path,
                                               )

#----------------------------------------------------------------------

def rev_update_switched_things(sbox):
  "reverse update switched wc things to an older rev"

  if sbox.build():
    return 1

  wc_dir = sbox.wc_dir
  
  # Setup some switched things (don't bother verifying)
  if do_routine_switching(wc_dir, 0):
    return 1

  # Commit some stuff (don't bother verifying)
  if commit_routine_switching(wc_dir, 0):
    return 1

  # Some convenient path variables
  iota_path = os.path.join(wc_dir, 'iota')
  B_path = os.path.join(wc_dir, 'A', 'B')

  # Update to HEAD (tested elsewhere)
  svntest.main.run_svn (None, 'up', wc_dir)

  # Now, reverse update, back to the pre-commit state.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='U '),
    'A/B/pi' : Item(status='U '),
    'A/B/Z' : Item(status='D '),
    })

  # Create expected disk tree
  expected_disk = get_routine_disk_state(wc_dir)
  expected_disk.tweak('A/D/gamma', contents="This is the file 'gamma'.apple")
  expected_disk.tweak('A/D/G/pi', contents="This is the file 'pi'.watermelon")
  expected_disk.add({
    'A/D/G/Z' : Item(),
    'A/D/G/Z/zeta' : Item("This is the file 'zeta'."),
    })
    
  # Create expected status tree for the update.
  expected_status = get_routine_status_state(wc_dir)
  expected_status.tweak(repos_rev=2, wc_rev=2)
  expected_status.tweak('iota', 'A/B', switched='S')
  expected_status.tweak('A/B', 'A/B/pi', 'A/B/rho', 'A/B/tau', 'iota',
                        wc_rev=1)
  expected_status.add({
    'A/D/G/Z' : Item(status='  ', wc_rev=2, repos_rev=2),
    'A/D/G/Z/zeta' : Item(status='  ', wc_rev=2, repos_rev=2),
    })

  return svntest.actions.run_and_verify_update(wc_dir,
                                               expected_output,
                                               expected_disk,
                                               expected_status,
                                               None, None, None,
                                               None, None, 1,
                                               '-r', '1',
                                               B_path,
                                               iota_path
                                               )


#----------------------------------------------------------------------

def log_switched_file(sbox):
  "show logs for a switched file"

  sbox.build()
  wc_dir = sbox.wc_dir
  
  # Setup some switched things (don't bother verifying)
  if do_routine_switching(wc_dir, 0):
    raise svntest.Failure  # really, do_routine_switching should raise this

  # edit and commit switched file 'iota'
  iota_path = os.path.join(wc_dir, 'iota')
  svntest.main.run_svn (None, 'ps', 'x', 'x', iota_path)
  svntest.main.run_svn (None, 'ci', '-m', 
                        'set prop on switched iota', 
                        iota_path)

  # log switched file 'iota'
  output, error = svntest.main.run_svn (None, 'log', iota_path)
  for line in output:
    if line.find("set prop on switched iota") != -1:
      break
  else:
    raise svntest.Failure


#----------------------------------------------------------------------

### ...and a slew of others...

########################################################################
# Run the tests


# list all tests here, starting with None:
test_list = [ None,
              routine_switching,
              commit_switched_things,
              full_update,
              full_rev_update,
              update_switched_things,
              rev_update_switched_things,
              log_switched_file,
              ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
