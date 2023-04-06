<script lang="ts">

export default {
  data() {
    var self = this;
    return {
      //
    };
  },
  mounted() {
    window.addEventListener("keydown", e => {
      // console.log(e.key);
      switch (e.key) {
        case 'Backspace': this.postGUICommand(-1); break;
        case 'Escape': this.postGUICommand(-1); break;
        case 'Enter': this.postGUICommand(0); break;
        case 'w': this.postGUICommand(1); break;
        case 'a': this.postGUICommand(3); break;
        case 's': this.postGUICommand(2); break;
        case 'd': this.postGUICommand(4); break;
      }
    });
  },
  methods: {
    async postGUICommand(arg: number) {
      // Simple PUT request with a JSON body using fetch
      const requestOptions = {
        method: "POST",
        headers: { 'Accept': 'application/json', "Content-Type": "application/json" },
        body: JSON.stringify({ gui_cmd: arg })
      };
      console.log(requestOptions);
      const rawResponse = await fetch("http://192.168.0.173/api/v1/gui_cmd", requestOptions);

      console.log(rawResponse);
    },

  },
};
</script>

<template>
  <div class="gui-control">
    <h1 class="guiControlTitle">GUI Control</h1>


    <div class="gui-control-btn">
      <div>
        <button id="gui-up-btn" class="btn" v-on:click="postGUICommand(1)">
          <i class="fa fa-arrow-up">UP</i>
        </button>
      </div>
      <button id="gui-left-btn" class="btn" v-on:click="postGUICommand(3)">
        <i class="fa fa-arrow-left">LEFT</i>
      </button>
      <button id="gui-right-btn" class="btn" v-on:click="postGUICommand(4)">
        <i class="fa fa-arrow-right">RIGHT</i>
      </button>
      <div>
        <button id="gui-down-btn" class="btn" v-on:click="postGUICommand(2)">
          <i class="fa fa-arrow-down">DOWN</i>
        </button>
      </div>
    </div>

  </div>
</template>

<style>
.gui-control-btn {
  display: flex;
  justify-content: space-between;
  margin: 0 0 0.4rem 0;
}

.guiControlTitle {
  color: rgb(255, 49, 49);
}

.gui-control {
  /* -ms-transform: translateY(-50%);
  transform: translateY(-50%); */
}
</style>
