return (!true === false) &&
  (!false === true) &&
  (!!12 === true) &&
  (!!0 === false) &&
  (!!NaN === false) &&
  (!!undefined === false) &&
  (!!null === false) &&
  (!!{} == true);
