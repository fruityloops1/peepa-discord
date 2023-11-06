#pragma once

constexpr const char sConflictFmt1[] = R"(
<!DOCTYPE html>
<html>
<head>
  <title>Solve Merge Conflict</title>
  <script>
    function clickAllOld() {
      var radioButtons = document.querySelectorAll('input[type="radio"][id=^="conflict1"]');
      radioButtons.forEach(function(radioButton) {
        radioButton.checked = true;
      });
    }
    function clickAllNew() {
      var radioButtons = document.querySelectorAll('input[type="radio"][id=^="conflict2"]');
      radioButtons.forEach(function(radioButton) {
        radioButton.checked = true;
      });
    }
  </script>
  <style>
    table {
      border-collapse: collapse;
    }
    
    th, td {
      padding: 5px;
    font-family: Arial;
    }
    
    label {
      font-family: Arial;
      display: inline-block;
      margin-right: 10px;
    }
    
    h2 {
        font-family: Arial;
    }
    
    input {
        font-family: Arial;
    }

    p {
        font-family: Arial;
    }
    
    .addr {
        max-width: 10vw;
        overflow: auto;
        white-space: nowrap;
    }
    
    .symbolname {
        max-width: 40vw;
        overflow: auto;
        white-space: nowrap;
    }

    .buttons {
      display: flex;
      gap: 10px;
    }
  </style>
</head>
<body>
  <h2>Solve Merge Conflict</h2>
  <p>Merge conflicts have been found. Please select the symbols of higher quality and click submit.</p>
  
  <div class="buttons">
    <button onclick=\"clickAllOld()\">Select all old</button>
    <button onclick=\"clickAllNew()\">Select all new</button>
  </div>
  <br><br>
  <form method="post" action="submit_resolution">
    <table>
      <tr>
        <th>Address</th>
        <th>Old Symbol</th>
        <th>New Symbol</th>
      </tr>
      )";
constexpr const char sConflictFmt2[] = R"(
    </table>
    
    <br><br>
    
    <input type="submit" value="Submit">
  </form>

  <div class="buttons>
    <form method="post" action="submit_resolution">
      <input type="hidden" name="cancel" value="SUPERDUPERCANCELCANCELTHISSHITSOWECANNOTMERGETHESYMBOLSATALL">
      <button type="submit">Cancel</button>
    </form>
  </div>
</body>
</html>
)";

constexpr const char sConflictEntryFmt[] = R"(
<tr>
  <td>
    <label>
      <div class="addr">71%.8X</div>
    </label>
  </td>
  <td>
    <label for="conflict1-%d">
      <div class="symbolname">
        <input type="radio" id="conflict1-%d" name="%d" value="old">
        %s
      </div>
    </label>
  </td>
  <td>
    <label for="conflict2-%d">
      <div class="symbolname">
        <input type="radio" id="conflict2-%d" name="%d" value="new" checked>
        %s
      </div>
    </label>
  </td>
</tr>
)";
