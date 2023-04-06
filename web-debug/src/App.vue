<script setup lang="ts">
import { RouterLink, RouterView } from "vue-router";
import Footer from "./components/Footer.vue";
</script>

<template>
  <header>

    <div class="wrapper">
      <nav>
        <!-- <RouterLink to="/">Home</RouterLink> -->
        <!-- <RouterLink to="/about">About</RouterLink>
        <RouterLink to="/shop">Shop</RouterLink>
        <RouterLink to="/contact">Contact</RouterLink> -->
      </nav>
    </div>
  </header>
  <RouterView />
  <Footer></Footer>
</template>

<style scoped>
header {
  line-height: 1.5;
  /* max-height: 100vh; */
}

.logo {
  display: block;
  margin: 0 auto;
}

nav {
  width: 100%;
  font-size: 12px;
  text-align: center;
  margin-top: 1rem;
}

nav a.router-link-exact-active {
  color: var(--color-text);
}

nav a.router-link-exact-active:hover {
  background-color: transparent;
}

nav a {
  display: inline-block;
  padding: 0 1rem;
  border-left: 1px solid var(--color-border);
}

nav a:first-of-type {
  border: 0;
}

@media (min-width: 500px) {
  header {
    /* display: flex; */
    /* place-items: center; */
    text-align: center;
    /* padding-right: calc(var(--section-gap) / 2); */
  }

  /* .logo {
    margin: 0 0 0 0;
  } */

  header .wrapper {
    text-align: center;
    /* display: flex; */
    /* place-items: flex-start; */
    /* flex-wrap: wrap; */
  }

  nav {
    /* text-align: center; */
    /* margin-left: -1rem;
    font-size: 1rem;

    padding: 1rem 0;
    margin-top: 1rem; */
  }


}
</style>
