var notyf = new Notyf({
  duration: 5000,
  position: {
    x: "right",
    y: "bottom",
  },
  types: [
    {
      type: "success",
      className: "wa-success",
      background: "var(--ll-sound-transit-green)",
      border: "1px solid var(--wa-color-border-quiet)",
      color: "var(--wa-color-on-quiet)",
      ripple: true,
    },
    {
      type: "error",
      background: "var(--wa-color-fill-quiet)",
      border: "1px solid var(--wa-color-border-quiet)",
      color: "var(--wa-color-on-quiet)",
      className: "wa-danger",
      ripple: true,
    },
  ],
});
