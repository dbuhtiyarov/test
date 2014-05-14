class ExpectedOutput:
  """Contains expected output, and performs comparisons."""
  is_regex = False
  is_unordered = False
  def __init__(self, output, match_all=True):
    """Initialize the expected output to OUTPUT which is a string, or a list
    of strings, or None meaning an empty list. If MATCH_ALL is True, the
    expected strings will be matched with the actual strings, one-to-one, in
    the same order. If False, they will be matched with a subset of the
    actual strings, one-to-one, in the same order, ignoring any other actual
    strings among the matching ones."""
    self.output = output
    return str(self.output)
    raise Exception('badness')

  def matches(self, other, except_re=None):
    """Return whether SELF.output matches OTHER (which may be a list
    of newline-terminated lines, or a single string).  Either value
    may be None."""
    if self.output is None:
      expected = []
    else:
      expected = self.output
    if other is None:
      actual = []
    else:
      actual = other

    if not isinstance(expected, list):
      expected = [expected]
    if except_re:
      return self.matches_except(expected, actual, except_re)
    else:
      return self.is_equivalent_list(expected, actual)

  def matches_except(self, expected, actual, except_re):
    "Return whether EXPECTED and ACTUAL match except for except_re."
    if not self.is_regex:
      i_expected = 0
      i_actual = 0
      while i_expected < len(expected) and i_actual < len(actual):
        if re.match(except_re, actual[i_actual]):
          i_actual += 1
        elif re.match(except_re, expected[i_expected]):
          i_expected += 1
        elif expected[i_expected] == actual[i_actual]:
          i_expected += 1
          i_actual += 1
        else:
          return False
      if i_expected == len(expected) and i_actual == len(actual):
            return True
      return False
    else:
      raise Exception("is_regex and except_re are mutually exclusive")

  def is_equivalent_list(self, expected, actual):
    "Return whether EXPECTED and ACTUAL are equivalent."
    if not self.is_regex:
      if self.match_all:
        # The EXPECTED lines must match the ACTUAL lines, one-to-one, in
        # the same order.
        return expected == actual

      # The EXPECTED lines must match a subset of the ACTUAL lines,
      # one-to-one, in the same order, with zero or more other ACTUAL
      # lines interspersed among the matching ACTUAL lines.
      i_expected = 0
      for actual_line in actual:
        if expected[i_expected] == actual_line:
          i_expected += 1
          if i_expected == len(expected):
            return True
      return False

    expected_re = expected[0]
    # If we want to check that every line matches the regexp
    # assume they all match and look for any that don't.  If
    # only one line matching the regexp is enough, assume none
    # match and look for even one that does.
      all_lines_match_re = True
    else:
      all_lines_match_re = False

    # If a regex was provided assume that we actually require
    # some output. Fail if we don't have any.
    if len(actual) == 0:
      return False
      if self.match_all:
        if not re.match(expected_re, actual_line):
          return False
      else:
        # As soon an actual_line matches something, then we're good.
        if re.match(expected_re, actual_line):

    return all_lines_match_re
    """Delegate to the display_lines() routine with the appropriate
    args.  MESSAGE is ignored if None."""
    display_lines(message, label, self.output, actual,
                  self.is_regex, self.is_unordered)
    ExpectedOutput.__init__(self, None, False)
  def is_equivalent_list(self, ignored, actual):
      print(message)
  is_regex = True
  """Marks unordered output, and performs comparisons."""
  is_unordered = True
  def __cmp__(self, other):
    raise Exception('badness')
  def matches_except(self, expected, actual, except_re):
    assert type(actual) == type([]) # ### if this trips: fix it!
    return self.is_equivalent_list([l for l in expected if not except_re.match(l)],
                                   [l for l in actual if not except_re.match(l)])
  def is_equivalent_list(self, expected, actual):
    "Disregard the order of ACTUAL lines during comparison."
    e_set = set(expected)
    a_set = set(actual)
    if self.match_all:
      if len(e_set) != len(a_set):
      if self.is_regex:
        for expect_re in e_set:
          for actual_line in a_set:
            if re.match(expect_re, actual_line):
              a_set.remove(actual_line)
              break
          else:
            # One of the regexes was not found
            return False
        return True
      # All expected lines must be in the output.
      return e_set == a_set
    if self.is_regex:
      # If any of the expected regexes are in the output, then we match.
      for expect_re in e_set:
        for actual_line in a_set:
          if re.match(expect_re, actual_line):
            return True
      return False
    # If any of the expected lines are in the output, then we match.
    return len(e_set.intersection(a_set)) > 0
class UnorderedRegexOutput(UnorderedOutput, RegexOutput):
  is_regex = True
  is_unordered = True
    print(message)
    print('EXPECTED %s:' % label)
    print('ACTUAL %s:' % label)
def display_lines(message, label, expected, actual, expected_is_regexp=None,
                  expected_is_unordered=None):
  with LABEL) followed by ACTUAL (also labeled with LABEL).
  Both EXPECTED and ACTUAL may be strings or lists of strings."""
    print(message)
    output = 'EXPECTED %s' % label
    if expected_is_regexp:
      output += ' (regexp)'
      expected = [expected + '\n']
    if expected_is_unordered:
      output += ' (unordered)'
    output += ':'
    print(output)
      sys.stdout.write(x)
    print('ACTUAL %s:' % label)
      sys.stdout.write(x)

  # Additionally print unified diff
  if not expected_is_regexp:
    print('DIFF ' + ' '.join(output.split(' ')[1:]))

    if type(expected) is str:
      expected = [expected]

    if type(actual) is str:
      actual = [actual]

    for x in unified_diff(expected, actual,
                          fromfile="EXPECTED %s" % label,
                          tofile="ACTUAL %s" % label):
      sys.stdout.write(x)
                              raisable=None, except_re=None):
  ExpectedOutput (and if not, it is wrapped as such).  RAISABLE is an
  actual = [line for line in actual if not line.startswith('DBG:')]
  if not expected.matches(actual, except_re):
    display_lines(message, "Exit Code",
                  str(expected) + '\n', str(actual) + '\n')