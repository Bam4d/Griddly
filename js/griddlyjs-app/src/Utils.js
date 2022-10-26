

module.exports = Object.freeze({
  hashString: (string) => {
    let hash = 0,
      i,
      chr;
    if (string.length === 0) return hash;
    for (i = 0; i < string.length; i++) {
      chr = string.charCodeAt(i);
      hash = (hash << 5) - hash + chr;
      hash |= 0; // Convert to 32bit integer
    }
    return hash;
  },

  getRandomHash: () => {
    return window.crypto.getRandomValues(new Uint32Array(1))[0];
  },
});
